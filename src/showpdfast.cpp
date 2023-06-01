#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <podofo/podofo.h>
#include <podofo/podofo.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "showpdfast.h"

#include <cassert>
#define ASSERT(test, msg) assert((test) && (msg))
#define TODO(msg) ASSERT(false, msg)
#define NOT_IMPLEMENTED() ASSERT(false, "Not Implemented...")
#define UNREACHABLE() ASSERT(false, "Unreachable code...")

// TODO(Sam): This is not thread safe yet...
PoDoFo::PdfMemDocument *opened_pdfs[256];
unsigned int next_free_pdf_id = 0;
#define PDF(id) opened_pdfs[(id)]

SPDFast::DeviceConfig SPDFast::get_device_config_for_page(unsigned int pdf_id, unsigned int page_id,
														  float device_display_left, float device_display_bottom, 
														  float device_display_width, float device_display_height,
														  bool apply_rotation)
{
	using namespace PoDoFo;

	constexpr float cos_rot[4] = {1.f, 0.f, -1.f, 0.f};
	constexpr float sin_rot[4] = {0.f, 1.f, 0.f, -1.f};

	float width_ratio, height_ratio;
	float origin_x, origin_y;
	
	PdfPage& page = PDF(pdf_id)->GetPages().GetPageAt(page_id);
	Rect crop_box = page.GetCropBox();

	// Rotation is 0 90 180 or 270
	int rotation = 0;
	if(apply_rotation) rotation = page.GetRotationRaw();

	switch(rotation)
	{
	case 0:
	{
		origin_x = device_display_left;
		origin_y = device_display_bottom;
		width_ratio = device_display_width/(float)crop_box.Width;
		height_ratio = device_display_height/(float)crop_box.Height;
		break;
	}
	case 90:
	{
		origin_x = device_display_left;
		origin_y = device_display_bottom + device_display_height;
		height_ratio = device_display_width/(float)crop_box.Height;
		width_ratio = device_display_height/(float)crop_box.Width;
		break;
	}
	case 180:
	{
		origin_x = device_display_left + device_display_width;
		origin_y = device_display_bottom + device_display_height;
		width_ratio = device_display_width/(float)crop_box.Width;
		height_ratio = device_display_height/(float)crop_box.Height;
		break;
	}
	case 270:
	{
		origin_x = device_display_left + device_display_width;
		origin_y = device_display_bottom;
		height_ratio = device_display_width/(float)crop_box.Height;
		width_ratio = device_display_height/(float)crop_box.Width;
		break;
	}
	default:
		UNREACHABLE();
	}

	int q = rotation / 90;
	
	SPDFast::DeviceConfig device;
	device.matrix_transform[0] = cos_rot[q]*width_ratio;
	device.matrix_transform[1] = -sin_rot[q]*height_ratio;
	device.matrix_transform[2] = sin_rot[q]*width_ratio;
	device.matrix_transform[3] = cos_rot[q]*height_ratio;
	device.matrix_transform[4] = origin_x;
	device.matrix_transform[5] = origin_y;
	return device;
}

void SPDFast::debug_print_device_config(const SPDFast::DeviceConfig* config)
{
	std::cout << "DeviceConfig <" << (std::size_t)(config) << ">:\n";
	std::cout << " | " << config->matrix_transform[0] << " " << config->matrix_transform[1] << " 0 |\n";
	std::cout << " | " << config->matrix_transform[2] << " " << config->matrix_transform[3] << " 0 |\n";
	std::cout << " | " << config->matrix_transform[4] << " " << config->matrix_transform[5] << " 1 |\n";
	std::cout << "\n";
}

// TODO(Sam): Test this...
void apply_transform_matrix_to_point(const float mat[6], float x, float y, float *x_out, float *y_out)
{
	// [a b c d e f]
	// Corresponds to
	//   a  b  0
	//   c  d  0
	//   e  f  1

	// We perform (as specified in 32000-1:2008 8.3.4)
	//                           |a  b  0|
	// [x', y', 1] = [x, y, 1] x |c  d  0|
	//                           |e  f  1|

	*x_out = x*mat[0] + y*mat[2] + mat[4];
	*y_out = x*mat[1] + y*mat[3] + mat[5];
}

// TODO(Sam): Test this...
void apply_transform_matrix_to_matrix(const float mat[6], const float transform[6], float mat_out[6])
{
	// [a b c d e f]
	// Corresponds to
	//   a  b  0
	//   c  d  0
	//   e  f  1

	// We perform (as specified in 32000-1:2008 8.3.4)
	// M' = M_T x M
	// where M_T is the new transformation and M is all previous transformations
	
	mat_out[0] = transform[0]*mat[0] + transform[1]*mat[2];
	mat_out[1] = transform[0]*mat[1] + transform[1]*mat[3];

	mat_out[2] = transform[2]*mat[0] + transform[3]*mat[2];
	mat_out[3] = transform[2]*mat[1] + transform[3]*mat[3];

	mat_out[4] = transform[4]*mat[0] + transform[5]*mat[2] + mat[4];
	mat_out[5] = transform[4]*mat[1] + transform[5]*mat[3] + mat[5];
}

struct TextState {
	float character_spacing; // Init to 0
	float word_spacing; // Init to 0
	float horizontal_scaling; // Init to 100.0
	float leading; // Unscaled text space units // Init to 0
	PoDoFo::PdfFont *text_font; // No initial value, shall be specified in BT/BE
	float text_font_size; // No initial value, shall be specified in BT/BE
	void *text_rendering_mode; // Init to mode 0 (should be an enum, 0 to 7)
	float text_rise; // Unscaled text space units // Init to 0
	bool text_knockout; // Init to true // No operator to set it, see 9.3.8 from ISO 3200-1 2008

	// TODO(Sam): Mapping function to PoDoFo::PdfTextState (check TextState)
};

struct DeviceIndependentGraphicsStateParams {
	// [a b c d e f]
	// Corresponds to
	//   a  b  0
	//   c  d  0
	//   e  f  1
	// e.g. translation is [1, 0, 0, 1, tx, ty]
	//      scaling is [sx, 0, 0, sy, 0, 0]
	float CTM[6]; // Init to transfo from default user space to device space

	void *cliping_path; // Init to boundary of entier imagable portion of output page
	void *color_space; // Init to DeviceGray
	void *color; // Init to black
	TextState text_state; // 9 parameters (e.g. font, scale, etc)
	float line_width; // Init to 1.0
	unsigned int line_cap; // Init to 0 (square butt cap)
	unsigned int line_join; // Init to 0 (mitered join)
	float miter_limit; // Init to 10.0 (for a miter cutoff of ~11.5°)
	void *dash_pattern; // Init to a solid line
	void *rendering_intent; // Init to /RelativeColorimetric // Look at PoDoFo::PdfRenderingIntent
	bool stroke_adjustment; // Init to false
	void *blend_mode; // Init to /Normal
	void *soft_mask; // Init to /None
	float alpha_constant; // Init to 1.0
	bool alpha_source; // Init to false
};

struct DeviceDependentGrahicsStateParams {
	bool overprint; // Init to false
	float overprint_mode; // Init to 0.0
	void *black_generation; // Init to suitable device dependent value
	void *undercolor_removal; // Init to suitable device dependent value
	void *transfer; // Init to suitable device dependent value
	void *halftone; // Init to suitable device dependent value
	float flatness; // Init to 1.0
	float smoothness; // Init to suitable device dependent value
};

struct GraphicsState {
	DeviceIndependentGraphicsStateParams device_independent;
	DeviceDependentGrahicsStateParams device_dependent;
};

bool SPDFast::open_pdf(char *filename, unsigned int *pdf_id)
{
	*pdf_id = next_free_pdf_id++;
	try
	{
		PoDoFo::PdfMemDocument* document = new PoDoFo::PdfMemDocument();
		document->Load(filename);
		opened_pdfs[*pdf_id] = document;
	}
	catch (PoDoFo::PdfError error)
	{
		// TODO(Sam): Proper error management
		std::cout << "** An error occured:" << error.what() << "\n";
		return false;
	}

	return true;
}

bool SPDFast::close_pdf(unsigned int pdf_id)
{
	if(opened_pdfs[pdf_id])
	{
		//opened_pdfs[pdf_id]->FreeObjectMemory(ref, force);
		delete opened_pdfs[pdf_id];
		opened_pdfs[pdf_id] = nullptr;
		return true;
	}

	return false;
}

void SPDFast::get_page_size(unsigned int pdf_id, unsigned int page_id, float *width, float *height, bool apply_rotation)
{
	using namespace PoDoFo;

	PdfPage& page = PDF(pdf_id)->GetPages().GetPageAt(page_id);
	Rect crop_box = page.GetCropBox();

	int rotation = 0;
	if(apply_rotation)
		rotation = page.GetRotationRaw();

	// If rotation is 0 or 180 the page rotation is already correct
	if(rotation / 90 % 2 == 0)
	{
		*width = (float)crop_box.Width;
		*height = (float)crop_box.Height;
	}
	// If rotation is 90 or 270 we have to invert width and height
	else
	{
		*height = (float)crop_box.Width;
		*width = (float)crop_box.Height;
	}
}

unsigned int SPDFast::get_page_count(unsigned int pdf_id)
{
	return PDF(pdf_id)->GetPages().GetCount();
}

// Note(Sam): We will need to propose some default conversion function for example
// to go from CIE-Based colour space to Device coulour. Those should be overridable
// by the user. I think of using some '#define' to do that.
// Default behaviour should be to display on a modern RGB screen

void SPDFast::debug_pdf_page(unsigned int pdf_id, unsigned int page_id)
{
	using namespace PoDoFo;
	PdfPage& page = PDF(pdf_id)->GetPages().GetPageAt(page_id);
	PdfFontManager& fonts = PDF(pdf_id)->GetFonts();
	PdfNameTree *name_tree = PDF(pdf_id)->GetNames();
	PdfIndirectObjectList& objects = PDF(pdf_id)->GetObjects();
	PdfResources& ressources = page.GetOrCreateResources();
	//name_tree->ToDictionary()

	// This is inspired by
	//void PdfPage::ExtractTextTo(vector<PdfTextEntry>& entries, const string_view& pattern, const PdfTextExtractParams& params) const;

	unsigned int nb_token = 0;
	unsigned int nb_op = 0;
		
	// Look FIGURE 4.1 Graphics objects
	PdfContentStreamReader reader(page);
	PdfContent content;
	while (reader.TryReadNext(content))
	{
		++nb_token;

		//std::cout << (content.Type == PdfContentType::Operator ? "Operator: " : "Other: ") << content.Keyword << "\n";
			
		switch (content.Type)
		{
		case PdfContentType::Operator:
		{
			++nb_op;
				
			if ((content.Warnings & PdfContentWarnings::InvalidOperator)
				!= PdfContentWarnings::None)
			{
				// Ignore invalid operators
				continue;
			}

			// NOTE(Sam): In PoDoFo, each content of type Operator contains a stack of Variant
			//            corresponding to each parameters of the operator.
			// PoDoFo::PdfOperator
			// PoDoFo::PdfVariantStack PoDoFo::PdfVariant
			//    PoDoFo::PdfDataType
			/*
			std::cout << " - content.Stack:" << "\n";
			for(unsigned int i = 0; i < content.Stack.size(); ++i)
				std::cout << "    " << (unsigned int)(content.Stack[i].GetDataType()) << " : " << content.Stack[i].GetDataTypeString() << " -> " << content.Stack[i].ToString() << "\n";
			*/
			
			// T_l TL: Set the text leading, T_l
			switch (content.Operator)
			{
			case PdfOperator::TL:
			{
				// TODO(Sam):...
				break;
			}
			case PdfOperator::cm:
			{
				// TODO(Sam):...
				break;
			}
			// t_x t_y Td     : Move to the start of the next line
			// t_x t_y TD     : Move to the start of the next line
			// a b c d e f Tm : Set the text matrix, T_m , and the text line matrix, T_lm
			case PdfOperator::Td:
			case PdfOperator::TD:
			case PdfOperator::Tm:
			{
				// TODO(Sam):...
				break;
			}
			// T*: Move to the start of the next line
			case PdfOperator::T_Star:
			{
				// TODO(Sam):...
				break;
			}
			// BT: Begin a text object
			case PdfOperator::BT:
			{
				// TODO(Sam):...
				// From spec we should reset the font state with default value
				// it is not allowed to have nested BT/ET keywords thus we don't have
				// to manage a stack as with Q/q
				break;
			}
			// ET: End a text object
			case PdfOperator::ET:
			{
				// TODO(Sam):...
				break;
			}
			// font size Tf : Set the text font, T_f
			case PdfOperator::Tf:
			{
				// TODO(Sam):...
				//PdfObject object = PdfObject(content.Stack[1]);

				PdfName fontName = content.Stack[1].GetName();
				const PdfFont *font = ressources.GetFont(fontName.GetString());
				PdfFontType font_type = font->GetType();
				std::cout << " FONT NAME: " << fontName.GetString() << "\n";
				std::cout << "> Font type: " << (unsigned int) font_type << "\n";
				std::cout << "> Loaded: " << font->IsObjectLoaded() << "\n";
				std::cout << "> Identifier: " << font->GetIdentifier().ToString() << "\n";
				std::cout << "> Name: " << font->GetName() << "\n";

				/*

				  NOTE: May be usefull to get data to render the font:
				  It is in PdfFontMetrics:
				bool HasFontFileData() const;

				/ ** Get an actual font data view
				 *
				 * The data shall be resident. For font coming from the /FontFile
				 * keys, GetFontFileObject() may also be available.
				 * \returns a binary buffer of data containing the font data
				 * /
				bufferview GetOrLoadFontFileData() const;

				/ ** Get direct access to the internal FreeType handle
				 *
				 *  \returns the internal freetype handle
				 * /
				bool TryGetOrLoadFace(FT_Face& face) const;
				FT_Face GetOrLoadFace() const;
				*/

				const PdfFontMetrics& metrics = font->GetMetrics();
				FT_Face font_face = metrics.GetOrLoadFace();
				std::cout << "FT family_name: " << font_face->family_name << "\n";
				
				//name_tree->GetValue(const PdfName& tree, const PdfString& key);

				//std::cout << (object.IsDictionary() ? "Dictionary" : "Not Dictionary") << "\n";
				//const PdfObject *parent = object.GetParent()->GetOwner();
				//std::cout << "Parent is " << (parent->IsDictionary() ? "Dictionary" : "Not Dictionary") << "\n";
				
				//auto& dict = object.GetDictionary();
				/*
				PdfObject* objTypeKey = dict.FindKey(PdfName::KeyType);
				if (objTypeKey == nullptr)
					std::cout <<  " --- Font: No Type" << "\n";

				if (objTypeKey->GetName() != "Font")
					std::cout << " --- InvalidDataType" << "\n";

				auto subTypeKey = dict.FindKey(PdfName::KeySubtype);
				if (subTypeKey == nullptr)
					std::cout << " --- Font: No SubType" << "\n";
				*/
				
				//std::unique_ptr<PdfFont> current_font;
				//PdfFont::TryCreateFromObject(object, current_font);

				/*
				//PdfFont &font = fonts.GetOrCreateFont(fontName);
				PdfFont *font = fonts.SearchFont("Arial");
				PdfFontType font_type = font->GetType();
				std::cout << "Font type: " << (unsigned int) font_type << "\n";
				std::cout << "Loaded: " << font->IsObjectLoaded() << "\n";
				std::cout << "Identifier: " << font->GetIdentifier().ToString() << "\n";
				std::cout << "Name: " << font->GetName() << "\n";
				*/
				break;
			}
			// string Tj : Show a text string
			// string '  : Move to the next line and show a text string
			// a_w a_c " : Move to the next line and show a text string,
			//             using a_w as the word spacing and a_c as the
			//             character spacing
			case PdfOperator::Tj:
			case PdfOperator::Quote:
			case PdfOperator::DoubleQuote:
			{
				// TODO(Sam):...
				break;
			}
			// array TJ : Show one or more text strings
			case PdfOperator::TJ:
			{
				// TODO(Sam):...
				break;
			}
			// Tc : word spacing
			case PdfOperator::Tc:
			{
				// TODO(Sam):...
				break;
			}
			case PdfOperator::Tw:
			{
				// TODO(Sam):...
				break;
			}
			// q : Save the current graphics state
			case PdfOperator::q:
			{
				// TODO(Sam):...
				break;
			}
			// Q : Restore the graphics state by removing
			// the most recently saved state from the stack
			case PdfOperator::Q:
			{
				// TODO(Sam):...
				break;
			}
			default:
			{
				// Ignore all the other operators
				break;
			}
			}

			break;
		}
		case PdfContentType::ImageDictionary:
		case PdfContentType::ImageData:
		{
			// Ignore image data token
			break;
		}
		case PdfContentType::DoXObject:
		{
			// TODO(Sam):...
			break;
		}
		case PdfContentType::EndXObjectForm:
		{
			// TODO(Sam):...
			break;
		}
		default:
		{
			throw std::runtime_error("Unsupported PdfContentType");
		}
		}
	}

	std::cout << "Found " << nb_op << "ops in " << nb_token << "toks on page " << (page_id+1) << "\n";
}
