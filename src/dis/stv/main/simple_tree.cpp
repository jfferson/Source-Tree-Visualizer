/*
 * simple_tree.cpp
 *
 *  Created on: Aug 12, 2010
 *      Author: programmer
 */

#include "../system/discursive_system.h"
#include "./configuration_agent.h"
#include "repository_access.h"
#include <Magick++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../dec/decorator_factory.h"
#include "../gen/digitizer_factory.h"
#include "../model/surrogate_tree_node.h"
#include "../dec/spatial_displacement.h"
#include "../draw/scanline_artist.h"
#include "../gen/space_colonizer.h"

using namespace Magick;
DiscursiveDebugAgent debugAgent;


int main(int argc, char **argv)
{
	// initialize libmagick
	InitializeMagick(*argv);

	// initialize our timing class
	DiscursiveTime timerAgent("Entire SourceTreeVis Program");

	// initialize our configuration agent
	ConfigurationAgent configAgent;

	// parse our config file and also pre-check command line
	configAgent.parseConfigFile();

	// parse our command line
	configAgent.parseCommandLine(argc,argv);

	// Tree Generation pipeline
	// create an abstract method for repository access
	RepositoryAccess* git = configAgent.initializeRepositoryType();

	// set our scaling factor from our received or default options
	double widthRescaling = configAgent.returnOptionByName("imageWidth")/(double)configAgent.returnOptionByName("startWidth");
	double heightRescaling = configAgent.returnOptionByName("imageHeight")/(double)configAgent.returnOptionByName("startHeight");

    // set our repository single or manyjpg options
	git->snapshotJpgs = configAgent.returnOptionByName("manyJpgs");
	git->jpgIndex = configAgent.returnOptionByName("outputFileNumber");
	git->scaleHeight = heightRescaling;
	git->scaleWidth = widthRescaling;
	git->fileName = configAgent.returnFileName();
	git->imageHeight = configAgent.returnOptionByName("imageHeight");
	git->imageWidth = configAgent.returnOptionByName("imageWidth");
	git->globalInserts = 0;
	git->localInserts = 0;
	git->insertTarget = 0;
	git->revJpgs = configAgent.returnOptionByName("revJpgs");
	git->globalRevs = 0;
	git->localRevs = 0;
	git->revTarget = 0;
	git->currentTreeSize = 0;
	git->logGenerated = 0;

	// retrieve our source tree
	DiscursiveDebugPrint("default","Retrieving From Server, Please Wait...\n");
	timerAgent.Tic("Initial Retrieve");
	git->source = git->retrieve();
	DiscursiveDebugPrint("time","Server Retrieval Took:\n");
	timerAgent.Toc("Initial Retrieve");

	// output tree info
	DiscursiveDebugPrint("default","Source tree name is %s\n", git->source->data[TreeNodeKey::NAME].c_str());
	DiscursiveDebugPrint("default","Source tree has %d revisions which require %d tree inserts\n",git->globalRevs,git->globalInserts);

	delete(git->source);
	git->source = NULL;

	// create loop step variables generically for any jpg looping type
	int loopStart = 0;
	int loopStop = 1;
	int loopStep = 1;
	int executeLoopType = 1;

	// check for our custom loop types
	if(configAgent.returnOptionByName("manyJpgs"))
	{
		loopStart = configAgent.returnOptionByName("jpgStart");
		loopStop = configAgent.returnOptionByName("jpgStop");
		loopStep = configAgent.returnOptionByName("jpgStep");
		executeLoopType = 2;
	}
	else if(configAgent.returnOptionByName("revJpgs"))
	{
		loopStart = configAgent.returnOptionByName("revStart");
		loopStop = configAgent.returnOptionByName("revStop");
		loopStep = configAgent.returnOptionByName("revStep");
		executeLoopType = 3;
	}

	// check for too many global inserts or revisions
	if((executeLoopType == 2) && (loopStop >= git->globalInserts))
	{
		loopStop = git->globalInserts-1;
	}
	DiscursiveDebugPrint("Total revs in repo = %d and loopstop = %d\n",git->globalRevs,loopStop);
	if((executeLoopType == 3) && (loopStop >= git->globalRevs))
	{
		loopStop = git->globalRevs-1;
	}

	// loop over a bunch of increasingly large trees for debug or revision tree generation
	// use our user-set parameters to define our step
	for(int i = loopStart; i<loopStop;i+= loopStep)
	{

		// reset our insert variables
		if(executeLoopType == 2)
		{
			git->localInserts = 0;
			git->insertTarget = i;
		}
		else if(executeLoopType == 3)
		{
			git->localRevs = 0;
			git->revTarget = i;
		}

		// retrieve our source tree
		git->source = git->retrieve();


		// Decorate surrogate tree nodes with locations
		DiscursiveDebugPrint("default","Decorating surrogate trees %d out of %d step value %d\n",i,loopStop,loopStep);
		timerAgent.Tic("Decorating surrogate trees");
//		SpatialDisplacement* disp = new SpatialDisplacement(git->imageWidth,git->imageHeight,scaleWidth,scaleHeight);
		Decorator* decorator = DecoratorFactory::getInstance(DecoratorFactory::SPATIAL_DISPLACEMENT_NAIVE, 4, git->imageWidth,git->imageHeight,widthRescaling,heightRescaling);
		decorator->decorate(git->source);
		timerAgent.Toc("Decorating surrogate trees");

		// Digitize decorated surrogate tree into line segment tree
		DiscursiveDebugPrint("default","Digitizing decorated surrogate trees into line segment trees %d out of %d step value %d\n",i,loopStop,loopStep);
		timerAgent.Tic("Digitizing decorated trees into line segments");
		int segmentLength = 1;
//	    SpaceColonizer *digitizer = new SpaceColonizer(segmentLength);
		Digitizer* digitizer = DigitizerFactory::getInstance(DigitizerFactory::SPACE_COLONIZER,1,segmentLength);
		DrawableData* lines = digitizer->digitize(git->source);
		timerAgent.Toc("Digitizing decorated trees into line segments");

		// Draw tree
		DiscursiveDebugPrint("default","Drawing Tree %d out of %d step value %d\n",i,loopStop,loopStep);
		timerAgent.Tic("Drawing Tree with artist.draw");
		Image canvas(Geometry(git->imageWidth,git->imageHeight),"white");
		ScanlineArtist artist;
		artist.draw(canvas, lines);
		timerAgent.Toc("Drawing Tree with artist.draw");

		// actually generate a tree
		timerAgent.Tic("actually generating image from canvas");
		git->WriteJPGFromCanvas(&canvas);
		timerAgent.Toc("actually generating image from canvas");

		delete digitizer;
		delete decorator;

		DiscursiveDebugPrint("default","\n");
	}

	// finish the timing on our entire program
	timerAgent.Toc("Entire SourceTreeVis Program");
	timerAgent.PrintRunningTotals();
	return 0;
}
