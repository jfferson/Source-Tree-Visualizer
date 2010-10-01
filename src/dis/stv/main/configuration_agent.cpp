/*
 * configuration_agent.cpp
 *
 *  Created on: Oct 1st, 2010
 *      Author: Hunter Davis
 */

#include "configuration_agent.h"


// -------------------------------------------------------------------------
// API :: ConfigurationAgent::ConfigurationAgent
// PURPOSE :: instantiation of ConfigurationAgent
//         :: defaults all values
// PARAMETERS :: None
// RETURN :: None
// -------------------------------------------------------------------------
ConfigurationAgent::~ConfigurationAgent()
{
	delete agentName;
};
// -------------------------------------------------------------------------
// API :: ConfigurationAgent::ConfigurationAgent
// PURPOSE :: instantiation of ConfigurationAgent
//         :: defaults all values
// PARAMETERS :: None
// RETURN :: None
// -------------------------------------------------------------------------
ConfigurationAgent::ConfigurationAgent()
{
	// agentName equates to the passed command line parameter argument string
	agentName = NULL;

	// type of agent to use, ie command line options or interactive
	agentType = 0;

	// image size options for jpg output
	imageWidth = 500;
	imageHeight = 500;

	// scaling factors to use for spatial displacement
	// Default is unity
	startWidth = imageWidth;
	startHeight = imageHeight;

	// our filename for output file
	fileName = "tree.jpg";
	outputFileNumber = 1000;

	// should we make many jpgs?
	manyJpgs = 0;

	// options for many jpgs
	jpgStep = 1;
	jpgStart = 3;
	jpgStop = 10000;

	// should we output a jpg per revision?
	revJpgs = 0;

	// options for jpg per revison
	revStep = 1;
	revStart = 2;
	revStop = 10000;

};

// -------------------------------------------------------------------------
// API :: ConfigurationAgent::display_usage
// PURPOSE :: outputs usage options for simple_tree
//         ::
// PARAMETERS :: None
// RETURN :: None
// -------------------------------------------------------------------------
void ConfigurationAgent::displayUsage( void )
{
	std::string usage_string = "\nsimple_tree [option] [optionstringorint] \n";
	usage_string += "where option can be any of the following";
	usage_string += "\n-g option - expects ~/Projects/source_tree_vis\n";
	usage_string += "\n-G option - expects gitHubusername:gitHubprojectname\n";
	usage_string += "\n-S option - expects http://hkit.googlecode.com/svn/trunk/\n";
	usage_string += "\n-C option - expects pserver:anonymous@bnetd.cvs.sourceforge.net:/cvsroot/bnetd\n";
	usage_string += "\n-i option - interactive mode (asks you questions) \n";
	usage_string += "\n-d option - debug level, defaults to 1\n";
	usage_string += "\n-n option - image numbering value, defaults to 1000\n";
	usage_string += "\n-o or -O option - output file name, defaults to tree.jpg\n";
	usage_string += "\n-m option - output the creation of the current tree one step at a time via many .images in sequence\n";
	usage_string += "\n--------------also expects the following start:stop:step i.e. 1:400:5       \n";
	usage_string += "\n----------------start number for many jpg tree rendering, default is 3\n";
	usage_string += "\n----------------finish number for many jpg tree rendering, default is treesize\n";
	usage_string += "\n----------------step value for many jpg tree rendering, default is 1\n";
	usage_string += "\n-R and -r option -output the creation of the current tree one revision at a time via many .images in sequence\n";
	usage_string += "\n-------------- -R also expects the following start:stop:step i.e. 1:400:5       \n";
	usage_string += "\n----------------start number for many jpg tree rendering, default is 3\n";
	usage_string += "\n----------------finish number for many jpg tree rendering, default is currentrevision\n";
	usage_string += "\n----------------step value for many jpg tree rendering, default is 1\n";
	usage_string += "\n-l option - spatial displacement scaling levels, expects W1:H1:W2:H2 defaults to 5000:5000:4500:4200\n";
	usage_string += "\n----------------where W1 is the start width\n";
	usage_string += "\n----------------where H1 is the start height\n";
	usage_string += "\n-z option - image size , expects WxH, defaults to 500x500 \n";


	DiscursivePrint("%s",usage_string.c_str());
    DiscursiveError("Copyright Discursive Labs LLC, 2010");
};

// -------------------------------------------------------------------------
// API :: ConfigurationAgent::returnAgentName
// PURPOSE :: returns the agent name
//         ::
// PARAMETERS :: None
// RETURN :: std::string agentName - name of agent
// -------------------------------------------------------------------------
std::string ConfigurationAgent::returnAgentName()
{
	return agentName;
}

// -------------------------------------------------------------------------
// API :: ConfigurationAgent::returnFileName
// PURPOSE :: returns the file name
//         ::
// PARAMETERS :: None
// RETURN :: std::string fileName - name of file to output
// -------------------------------------------------------------------------
std::string ConfigurationAgent::returnFileName()
{
	return fileName;
}

// -------------------------------------------------------------------------
// API :: ConfigurationAgent::parseConfigFile
// PURPOSE :: sets values based on config file input
//         ::
// PARAMETERS :: none
// RETURN :: None
// -------------------------------------------------------------------------
void ConfigurationAgent::parseConfigFile()
{

}
// -------------------------------------------------------------------------
// API :: ConfigurationAgent::parseCommandLine
// PURPOSE :: sets values based on command line input
//         ::
// PARAMETERS :: int argc, char ** argv - the cli arguments to the main program
// RETURN :: None
// -------------------------------------------------------------------------
void ConfigurationAgent::parseCommandLine(int argc, char **argv)
{
	// our option string
	static const char *optString = "g:G:S:C:O:o:m:R:l:z:n:ridh?";

	// loop over our command options in the normal unix way

	int opt;
	opt = getopt( argc, argv, optString );
	while( opt != -1 ) {
		switch( opt ) {
			case 'g':
				agentName = optarg;
				agentType = 1;
				break;
			case 'G':
				agentName = optarg;
				agentType = 2;
				break;
			case 'S':
				agentName = optarg;
				agentType = 3;
				break;
			case 'C':
				agentName = optarg;
				agentType = 4;
				break;
			case 'l':
				sscanf(optarg,"%d:%d",&startWidth,&startHeight);
				break;
			case 'z':
				sscanf(optarg,"%dx%d",&imageWidth,&imageHeight);
				break;
			case 'm':
				manyJpgs = 1;
				sscanf(optarg,"%d:%d:%d",&jpgStart,&jpgStop,&jpgStep);
				if(jpgStart < 3)
				{
					jpgStart = 3;
				}
				break;
			case 'n':
				sscanf(optarg,"%d",&outputFileNumber);
				break;
			case 'R':
				revJpgs = 1;
				sscanf(optarg,"%d:%d:%d",&revStart,&revStop,&revStep);
				if(revStart < 3)
				{
					revStart = 3;
				}
				break;
			case 'r':
				revJpgs = 1;
				revStart = 3;
				revStop = 100000;
				revStep = 1;
				break;
			case 'o':
			case 'O':
				fileName = optarg;
				break;
			case 'd':
				// set the debug level
				SetDiscursiveDebugLevel(1);
				break;
			case 'i':
				// set the interactivity level
				agentType = 5;
				break;
			case 'h':   /* fall-through is intentional */
			case '?':
				displayUsage();
				break;
			default:
				DiscursiveError("bad cli, bad!");
				break;
		}
		// get the next Command Line option
		opt = getopt( argc, argv, optString );
	}
}

// -------------------------------------------------------------------------
// API :: ConfigurationAgent::returnOptionByName
// PURPOSE :: returns the integer option value by std::string name
//         ::
// PARAMETERS :: std::string optionvalue - the name of value to retrieve
// RETURN :: int value - the value retrieved or an error code
// -------------------------------------------------------------------------
int ConfigurationAgent::returnOptionByName(std::string optionName)
{
	if(optionName == "agentType")
	{
		return agentType;
	}
	else 	if(optionName == "imageWidth")
	{
		return imageWidth;
	}
	else 	if(optionName == "imageHeight")
	{
		return imageHeight;
	}
	else 	if(optionName == "startWidth")
	{
		return startWidth;
	}
	else 	if(optionName == "startHeight")
	{
		return startHeight;
	}
	else 	if(optionName == "outputFileNumber")
	{
		return outputFileNumber;
	}
	else 	if(optionName == "manyJpgs")
	{
		return manyJpgs;
	}
	else 	if(optionName == "jpgStep")
	{
		return jpgStep;
	}
	else 	if(optionName == "jpgStart")
	{
		return jpgStart;
	}
	else 	if(optionName == "jpgStop")
	{
		return jpgStop;
	}
	else 	if(optionName == "revJpgs")
	{
		return revJpgs;
	}
	else 	if(optionName == "revStep")
	{
		return revStep;
	}
	else 	if(optionName == "revStart")
	{
		return revStart;
	}
	else 	if(optionName == "revStop")
	{
		return revStop;
	}
	else
	{
		// rufus: what's your favorite number?
	    DiscursiveError("Failure to lookup key value" + optionName);
		return -69;
	}

};

// -------------------------------------------------------------------------
// API :: ConfigurationAgent::initializeRepositoryType
// PURPOSE :: initializes the repository type based on initial values
//         ::
// PARAMETERS :: none
// RETURN :: RepositoryAccess* - repository access type
// -------------------------------------------------------------------------
RepositoryAccess* ConfigurationAgent::initializeRepositoryType()
{
	// let an interactive or command line agent create the repository access type
	switch(agentType)
	{
		case 5:
			return interactive_agent();
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			DiscursiveDebugPrint("%s\n",agentName);
			return noninteractive_agent(agentType, agentName);
			break;
		case 0:
			displayUsage();
			exit(0);
			break;
        default:
            /* You won't actually get here. */
        	DiscursiveError("an impossibility occured in error processing");
        	exit(1);
            break;
	}
};
