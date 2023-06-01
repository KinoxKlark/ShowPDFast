#include <iostream>
#include "raylib.h"
#include "showpdfast.h"

unsigned int current_pdf_id;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 450;

void draw_page_sheet(unsigned int page_id)
{
	// TODO(Sam): This should compute the device ctm and send it to the pdfast
	
	float ratio = 1.f;
	float width, height;
	SPDFast::get_page_size(current_pdf_id, page_id, &width, &height);

	float padding = 10.f;
	if(width > WINDOW_WIDTH - 2*padding)
	{
		ratio = (float) (WINDOW_WIDTH-2*padding) / width;
		width = (float) (WINDOW_WIDTH-2*padding);
		height = ratio*height;
	}
	if(height > WINDOW_HEIGHT - 2*padding)
	{
		ratio = (float) (WINDOW_HEIGHT - 2*padding) / height;
		height = (float) (WINDOW_HEIGHT - 2*padding);
		width = ratio*width;
	}

	int posX = (int)((float)WINDOW_WIDTH/2 - width/2);
	int posY = (int)((float)WINDOW_HEIGHT/2 - height/2);

	SPDFast::DeviceConfig config;
	config = SPDFast::get_device_config_for_page(current_pdf_id, page_id, (float)posX, (float)posY, width, height, true);
	SPDFast::debug_print_device_config(&config);

	DrawRectangle(posX, posY, (int)width, (int)height, RAYWHITE);
}

int main() {

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ShowPDFast Demo");
	SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_TRANSPARENT);
	std::cout << " ------------------------------ \n";

	if(!SPDFast::open_pdf("test.pdf", &current_pdf_id))
	{
		std::cout << "We couldn't read the pdf file..." << "\n";
		return -1;
	}

	std::cout << "PDF id: " << current_pdf_id << "\n";
	std::cout << " - " << SPDFast::get_page_count(current_pdf_id) << " pages" << "\n";

	SPDFast::debug_pdf_page(current_pdf_id, 0);
	
    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(GRAY);

			draw_page_sheet(0);
			//draw_page_sheet(1);

        EndDrawing();
    }

    CloseWindow();

	// Free ressrouces...
	SPDFast::close_pdf(current_pdf_id);

	return 0;
}
