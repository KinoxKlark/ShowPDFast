#include <iostream>
#include <string>
#include <podofo/podofo.h>

#include "showpdfast.h"

void SPDFast::renderPDF(char *filename)
{
	PoDoFo::PdfMemDocument document;
	
	try
	{
		document.Load(filename);

		PoDoFo::PdfPageCollection& pages = document.GetPages();
	
		std::cout << "Pdf '" << filename << "' has " << pages.GetCount() << " pages." << "\n";

	}
	catch (PoDoFo::PdfError error)
	{
		// TODO(Sam): Proper error management
		std::cout << "** An error occured:" << error.what() << "\n";
	}

}
