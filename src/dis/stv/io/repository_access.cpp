/*
 * repository_access.cpp
 *
 *  Created on: Sept 22, 2010
 *      Author: HunterD
 */


#include "repository_access.h"
#include <Magick++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../model/surrogate_tree_node.h"
#include "../dec/spatial_displacement.h"
#include "../draw/scanline_artist.h"
#include "../gen/space_colonizer.h"
using namespace Magick;


int RepositoryAccess::WriteJPGFromCanvas(Image* canvas)
{
	// Tree image size parameters
	// Generate Image Name with preceding integer and increment it
	std::stringstream integerPlusFileName;
	integerPlusFileName << "./out/";
	integerPlusFileName << this->jpgIndex;
	integerPlusFileName << this->fileName;
	this->jpgIndex++;

	try
	{
		// Write the image to a file with a preceding integer
		canvas->write(integerPlusFileName.str().c_str());
	}
	catch( Exception &error_ )
	{
		DiscursiveError(error_.what());
	}

	return 0;
};