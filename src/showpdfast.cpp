#include <iostream>
#include <string>
#include <podofo/podofo.h>

#include "showpdfast.h"

void SPDFast::renderPDF(char *filename)
{
	using namespace PoDoFo;
	
	PdfMemDocument document;

	unsigned int page_idx = 4;
	
	try
	{
		document.Load(filename);

		PdfPageCollection& pages = document.GetPages();
		PdfPage& page = pages.GetPageAt(page_idx);

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

				// PoDoFo::PdfOperator
				std::cout << "- Operator: " << unsigned int(content.Operator) << "\n";

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

		
		std::cout << "Pdf '" << filename << "' has " << pages.GetCount() << " pages." << "\n";
		std::cout << "Found " << nb_op << "ops in " << nb_token << "toks on page " << (page_idx+1) << "\n";

	}
	catch (PoDoFo::PdfError error)
	{
		// TODO(Sam): Proper error management
		std::cout << "** An error occured:" << error.what() << "\n";
	}

}
