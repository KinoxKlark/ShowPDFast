#ifndef SHOWPDFAST_H
#define SHOWPDFAST_H

// TODO(Sam): #define PDFAST_REAL float, PDFAST_ID unsigned int, etc...

namespace SPDFast {

	struct DeviceConfig {
		float matrix_transform[6];
	};

	// Remember to close pdf to free allocated memory
	bool open_pdf(char *filename, unsigned int *pdf_id);
	bool close_pdf(unsigned int pdf_id);

	// There is no guard on the pdf_id, we assume user knows what he/she is doing
	unsigned int get_page_count(unsigned int pdf_id);

	// There is not guard on the pdf_id or page_id, we assume user knows what he/she is doing
	// page_id goes from 0 to page_count-1
	void get_page_size(unsigned int pdf_id, unsigned int page_id, float *width, float *height, bool apply_rotation = true);

	DeviceConfig get_device_config_for_page(unsigned int pdf_id, unsigned int page_id,
											float device_display_left, float device_display_bottom, 
											float device_display_width, float device_display_height,
											bool apply_rotation = true);

	void debug_pdf_page(unsigned int pdf_id, unsigned int page_id);
	void debug_print_device_config(const DeviceConfig* config);
};

#endif // SHOWPDFAST_H
