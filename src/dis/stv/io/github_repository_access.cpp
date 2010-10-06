/*
 * git_repository_access.cpp
 *
 *  Created on: Aug 12, 2010
 *      Author: Mark Christensen
 */

#include "github_repository_access.h"

using namespace std;


// -------------------------------------------------------------------------
// API :: GitHubRepositoryAccess::GitRepositoryAccess
// PURPOSE :: instantiation of GitRepositoryAccess
//         :: specifically, with a github url of project info
// PARAMETERS :: std::string gitHubUserName, std::string gitHubProjectName
// RETURN :: None
// -------------------------------------------------------------------------
GitHubRepositoryAccess::GitHubRepositoryAccess(std::string gitHubUserName,std::string gitHubProjectName)
{
	// make project root the yaml api info stream for github
    std::stringstream projectRootStream;
    //http://github.com/api/v2/yaml/repos/show/schacon/grit
	projectRootStream << "http://github.com/api/v2/yaml/repos/show/";
	projectRootStream << gitHubUserName;
	projectRootStream << "/";
	projectRootStream << gitHubProjectName;
	this->root = projectRootStream.str();

	// add username and project name credentials to data structure
	this->userNameCredentials = gitHubUserName;
	this->repoNameCredentials = gitHubProjectName;

	// debug output for user credentials
	DiscursiveDebugPrint("github,repository access","\nGitHub User Name: %s, GitHub Project Name: %s\n",gitHubUserName.c_str(),gitHubProjectName.c_str());

	// add git repo type of github
	this->repoType = 2;

}

void GitHubRepositoryAccess::InsertByPathName(SurrogateTreeNode* tree, string pathname, long time)
{
	// Split off first part of path
	int firstIndex = pathname.find("/");
	if(firstIndex == 0)
	{
		// Our path started with a slash but we already have a root
		// So remove first slash and ignore
		pathname = pathname.substr(1,pathname.length()-1);
		firstIndex = pathname.find("/");
	}
	if(firstIndex == -1)
	{
		// We have no more path left.  Just a single entry (leaf)
		SurrogateTreeNode* file = new SurrogateTreeNode();
		string timeStr = boost::lexical_cast<string>(time);
		file->set(TreeNodeKey::CREATION_TIME, timeStr);
		file->set(TreeNodeKey::NAME, pathname);
		DiscursiveDebugPrint("github,repository access","Adding node '%s' @ time %ld\n",pathname.c_str(),time);
		tree->children->push_back(file);
	}
	else
	{
		// Split first string out for this nodes name
		string name = pathname.substr(0,firstIndex);
		// Look for node in children
		SurrogateTreeNode* node = NULL;

		vector<SurrogateTreeNode*>::iterator iter = tree->children->begin();
		for(;iter != tree->children->end(); ++iter)
		{
			SurrogateTreeNode* local = *iter;
			string nameComp = local->data[TreeNodeKey::NAME];
			//DiscursiveDebugPrint("github,repository access","Comparing %s to %s\n",nameComp.c_str(),name.c_str());
			if(!nameComp.compare(name))
			{
				// Found node
				node = (*iter);
				// Update node time if necessary
				if(time < atol(node->data[TreeNodeKey::CREATION_TIME].c_str()))
				{
					DiscursiveDebugPrint("github,repository access","Updating time of node[\"%s\"] to %ld from %ld\n", name.c_str(), time, atol(node->data[TreeNodeKey::CREATION_TIME].c_str()));
					node->set(TreeNodeKey::CREATION_TIME, boost::lexical_cast<string>(time));
				}
				break;
			}
		}
		// If child not found, add new node
		if(node == NULL)
		{
			node = new SurrogateTreeNode();
			string timeStr = boost::lexical_cast<string>(time);
			node->set(TreeNodeKey::CREATION_TIME, timeStr);
			node->set(TreeNodeKey::NAME, name);
			DiscursiveDebugPrint("github,repository access","Adding node '%s' @ time %ld\n",name.c_str(),time);
			tree->children->push_back(node);
		}
		// Else, use found node

		// Parse rest of string
		this->InsertByPathName(node, pathname.substr(firstIndex+1,(pathname.length() - firstIndex - 1)),time);
	}
}


// -------------------------------------------------------------------------
// API :: GitRepositoryAccess::generateTreeFromGitHub
// PURPOSE :: generation function for ptree when using github as a medium
//         ::
// PARAMETERS ::
// RETURN :: SurrogateTreeNode* - containing tree values generated
// -------------------------------------------------------------------------
SurrogateTreeNode* GitHubRepositoryAccess::generateTreeFromGitHub()
{
	// sanity check
	if(this->repoType != 2)
	{
		// shouldn't get here
		// TODO: add error handling function with try-catch default
		DiscursiveError("bad repo type");
	}

	// Blank ptree
	SurrogateTreeNode* treeResult = new SurrogateTreeNode();
	treeResult->set(TreeNodeKey::NAME, TreeNodeKey::ROOT);

	// only generate the log in the first pass
	if(logGenerated == 0)
	{
		// first retrieve the base log file from which to retrieve sha1 hashes for commit blobs
		std::string topLevelApiUrl;
		topLevelApiUrl = "http://github.com/api/v2/yaml/commits/list/";
		topLevelApiUrl += userNameCredentials;
		topLevelApiUrl += "/";
		topLevelApiUrl += repoNameCredentials;
		topLevelApiUrl += "/";
		topLevelApiUrl += "master";

		// Write any errors in here
		static char errorBuffer[CURL_ERROR_SIZE];

		// Write all expected data in here
		static string buffer;

		// set up a lib curl instantiation
		CURL *curl;
		CURLcode result;

		// Create our curl handle
		curl = curl_easy_init();

		if (curl)
		{
			// Now set up all of the curl options
		   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
		   curl_easy_setopt(curl, CURLOPT_URL, topLevelApiUrl.c_str());
		   curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
		   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

		   // Attempt to retrieve the remote page
		   DiscursiveDebugPrint("github,repository access","\n curl is attempting to pull %s\n",topLevelApiUrl.c_str());
		   result = curl_easy_perform(curl);

		   // Always cleanup
		   curl_easy_cleanup(curl);

		   // Did we succeed?
		   if (result == CURLE_OK)
		   {
			   logGenerated = 1;
			   repoLog = buffer;
		   }
		}
	}
	   // create an instringstream object to linify the input ala terminal
	   std::istringstream topLevelSS(repoLog);

	   // print debug our github top level block
	   //DiscursiveDebugPrint("github,repository access",buffer);

	   // loop over the outer string stream object and search for sha1 keys
	   // keep track of line numbering
	   int linenum = 0;
	   // our top level line storage
	   std::string topLevelLine;
	   // sha1 storage
	   std::string topLevelSHA1;
	   while (getline (topLevelSS, topLevelLine))
	   {
		   linenum++;

		   // the "id:" identifier will be 2 for entries, 4 for parents
		   int idVal = topLevelLine.find("id:");
		   // if we have a entry that's not a parent entry
		   if(idVal == 2)
		   {
			   topLevelSHA1 = topLevelLine.substr(6,topLevelLine.size()-5);
			   //DiscursiveDebugPrint("github,repository access","\n sha1 %s\n",SHA1->c_str());
			   retrieveDetailedGitHubBlock(treeResult,&topLevelSHA1);
		   }
	   }


	// return the filled tree
	return treeResult;
}

// -------------------------------------------------------------------------
// API :: GitHubRepositoryAccess::retrieveDetailedGitHubBlock
// PURPOSE :: retrieves detailed git hub info page for a sha1 key
//         ::
// PARAMETERS :: std::string *SHA1 - sha1 key
//            :: std::string SurrogateTreeNode* treeResult - resultant tree
// RETURN :: SurrogateTreeNode* - containing tree values generated
// -------------------------------------------------------------------------
void GitHubRepositoryAccess::retrieveDetailedGitHubBlock(SurrogateTreeNode* treeResult,std::string *SHA1)
{
	// At this point we have the SHA1 key, now lets pull the filename and creation time

	// Write any errors in here
	static char errorBuffer[CURL_ERROR_SIZE];

	// Write all expected data in here
	static string buffer;

	// set up a lib curl instantiation
	CURL *curl;
	CURLcode result;

	// create this custom github api sha1 URL with strings
	std::string gitHubApiUrl = "http://github.com/api/v2/yaml/commits/show/";
	gitHubApiUrl += userNameCredentials;
	gitHubApiUrl += "/";
	gitHubApiUrl += repoNameCredentials;
	gitHubApiUrl += "/";
	gitHubApiUrl += SHA1->c_str();

	// Create our curl handle for the detailed info for the SHA key
	curl = curl_easy_init();

	if (curl)
	{
		// Now set up all of the curl options
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
		curl_easy_setopt(curl, CURLOPT_URL, gitHubApiUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

		// clear out the buffer to be sure
		buffer = "";

		// Attempt to retrieve the remote page
		DiscursiveDebugPrint("github,repository access","\n curl is attempting to pull %s\n",gitHubApiUrl.c_str());
		result = curl_easy_perform(curl);

		// Always cleanup
		curl_easy_cleanup(curl);

		// Did we succeed?
		if (result == CURLE_OK)
		{
		   //DiscursiveDebugPrint("github,repository access","DDDDDDDDDDDDDDDDD%s",buffer.c_str());
		   parseDetailedGitHubBlock(treeResult,&buffer);
		}
	}
}

// -------------------------------------------------------------------------
// API :: GitHubRepositoryAccess::parseDetailedGitHubBlock
// PURPOSE :: parses detailed git hub info page
//         ::
// PARAMETERS :: std::string *SHA1 - sha1 key
//            :: std::string SurrogateTreeNode* treeResult - resultant tree
// RETURN :: SurrogateTreeNode* - containing tree values generated
// -------------------------------------------------------------------------
void GitHubRepositoryAccess::parseDetailedGitHubBlock(SurrogateTreeNode* treeResult,std::string *buffer)
{
	// create an istringstream to parse the suboutput
	std::istringstream gitHubBlockSS(*buffer);

	if(revTarget > 0)
	{
		localRevs++;
	}
	else
	{
		globalRevs++;
	}

	// each item does contain the time, but it's stringified
	// TODO: add a time string parser using STRFTIME (not hard)
	// TODO: pull out times with filenames using time first tagging
	// TODO: time first tagging = collect time, apply it as "time" of commit till next, loop
	long earliestFileDate = 357603262;


	// our top level line storage
	std::string topLevelLine;

	// our individual filename storage
	std::string fileNameString;

	// loop over the detailed commit and find filenames
	while (getline (gitHubBlockSS, topLevelLine))
	{
		// the "filename:" identifier will be where we find the files
		int fileNameVal = topLevelLine.find("filename:");
		int fileValVal = fileNameVal + 10;

		// if we find filename: , we want to insert the file to our tree
		if(fileNameVal > 0)
		{
			fileNameString = topLevelLine.substr(fileValVal,topLevelLine.size()-fileValVal);
			// at this point we have the filename, use that for a query string for the correct date
			earliestFileDate = retrieveDateFromGitHubFileName(&fileNameString);

			DiscursiveDebugPrint("github,repository access","Inserting %s @ %ld\n",fileNameString.c_str(),earliestFileDate);

			// increase the number of global inserts by one
			if((insertTarget > 0) && (localInserts < insertTarget))
			{
				localInserts++;
				InsertByPathName(treeResult,fileNameString,earliestFileDate);
			}
			if((revTarget > 0) && (localRevs < revTarget))
			{
				InsertByPathName(treeResult,fileNameString,earliestFileDate);
			}
			if((insertTarget == 0) && (revTarget == 0))
			{
				globalInserts++;
				InsertByPathName(treeResult,fileNameString,earliestFileDate);
			}
		}

	}
}

// -------------------------------------------------------------------------
// API :: GitHubRepositoryAccess::retrieveDateFromGitHubFileName
// PURPOSE :: returns date from githubfilename
//         ::
// PARAMETERS :: std::string githubfilename - filename
// RETURN :: long - oldest date associated with filename
// -------------------------------------------------------------------------
long GitHubRepositoryAccess::retrieveDateFromGitHubFileName(std::string *gitHubFileName)
{
	// At this point we have the filename key, now lets pull the creation time
	long oldestFileDate = 1;

	// Write any errors in here
	static char errorBuffer[CURL_ERROR_SIZE];

	// Write all expected data in here
	static string buffer;

	// set up a lib curl instantiation
	CURL *curl;
	CURLcode result;

	// create this custom github api sha1 URL with strings
	std::string gitHubApiUrl = "http://github.com/api/v2/yaml/commits/list/";
	gitHubApiUrl += userNameCredentials;
	gitHubApiUrl += "/";
	gitHubApiUrl += repoNameCredentials;
	gitHubApiUrl += "/master/";
	gitHubApiUrl += gitHubFileName->c_str();

	// Create our curl handle for the detailed info for the SHA key
	curl = curl_easy_init();

	if (curl)
	{
		// Now set up all of the curl options
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
		curl_easy_setopt(curl, CURLOPT_URL, gitHubApiUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

		// clear out the buffer to be sure
		buffer = "";

		// Attempt to retrieve the remote page
		DiscursiveDebugPrint("github,repository access","\n curl is attempting to pull %s\n",gitHubApiUrl.c_str());
		result = curl_easy_perform(curl);

		// Always cleanup
		curl_easy_cleanup(curl);

		// Did we succeed?
		if (result == CURLE_OK)
		{
		   //DiscursiveDebugPrint("github,repository access","DDDDDDDDDDDDDDDDD%s",buffer.c_str());
			oldestFileDate = parseDetailedGitHubFileBlock(&buffer);
		}
	}

	// return the oldest file date
	return oldestFileDate;
}

// -------------------------------------------------------------------------
// API :: GitHubRepositoryAccess::parseDetailedGitHubFileBlock
// PURPOSE :: returns date from githubfilename block
//         ::
// PARAMETERS :: std::string buffer - filename detailed checkout info
// RETURN :: long - oldest date associated with filename
// -------------------------------------------------------------------------
long GitHubRepositoryAccess::parseDetailedGitHubFileBlock(std::string *buffer)
{
	// create an istringstream to parse the suboutput for the oldest time
	std::istringstream gitHubTimeBlockSS(*buffer);

	// our individual filename date entire line storage
	std::string dateLine;

	// storage for the date string exactly
	std::string dateOnlyString;

	// did we find a date value
	bool dateFound = 0;

	// loop over the detailed commit and find filenames
	while (getline (gitHubTimeBlockSS, dateLine))
	{
		// the "filename:" identifier will be where we find the files
		int fileNameDateVal = dateLine.find("date:");
		int fileDateOffsetVal = fileNameDateVal + 7;

		// if we find date: , we want to keep it
		if(fileNameDateVal > 0)
		{
			dateFound = 1;
			dateOnlyString = dateLine.substr(fileDateOffsetVal,dateLine.size()-fileDateOffsetVal-7);
		}
	}

	if (dateFound == 1)
	{
		//DiscursiveDebugPrint("github,repository access","\nDDDDDD|||%s|||",dateOnlyString.c_str());

		// return the results of parsing this date string
		return parseExactDateString(&dateOnlyString);
	}
	else
	{
		return 0;
	}

}

// -------------------------------------------------------------------------
// API :: GitHubRepositoryAccess::parseExactDateString
// PURPOSE :: returns date from githubfilename block
//         ::
// PARAMETERS :: std::string buffer - exact date of format
//            :: 2007-10-09T23:18:20
// RETURN :: long - oldest date associated with filename
// -------------------------------------------------------------------------
long GitHubRepositoryAccess::parseExactDateString(std::string *buffer)
{
	// our time tm separated unix structure
	struct tm timeStructure;

	// our raw time structure
	time_t rawTime;

	// set our locale to US
	setlocale(LC_TIME, "en_US.iso88591");

	//strptime for the value
	//strptime("2007-10-09T23:18:20", "%Y-%m-%dT%H:%M:%S", &t);
	strptime(buffer->c_str(), "%Y-%m-%dT%H:%M:%S", &timeStructure);

	rawTime = mktime(&timeStructure);
	//DiscursiveDebugPrint("github,repository access","RAWTIME\n%ld\n",(long)rawTime);
	return (long) rawTime;
}


SurrogateTreeNode* GitHubRepositoryAccess::retrieve()
{
	SurrogateTreeNode* result = NULL;

	if(this->repoType == 2)
	{
		result = this->generateTreeFromGitHub();
	}

	return result;
}

// -------------------------------------------------------------------------
// API :: GitHubRepositoryAccess::writer
// PURPOSE :: callback function for curl to write buffers to
//         ::
// PARAMETERS :: char *data, size_t size, size_t nmemb, std::string *buffer
// RETURN :: None
// -------------------------------------------------------------------------
int GitHubRepositoryAccess::writer(char *data, size_t size, size_t nmemb, std::string *buffer)
{
   // What we will return
   int result = 0;

   // Is there anything in the buffer?
   if (buffer != NULL)
   {
     // Append the data to the buffer
     buffer->append(data, size * nmemb);

     // How much did we write?
     result = size * nmemb;
   }
   return result;
}


