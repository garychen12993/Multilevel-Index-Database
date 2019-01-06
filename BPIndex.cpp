/******************************************************************************
* B+ Tree Index Storage
*
* This program has four functions. The first function is to create a B+
* tree index from a given data in a text file. The second function is to list
* the contents of the record file, using the index as a "pointer" to the
* position in the record file. The third is to find a specific record by key.
* The fourth is to insert a new record into the record file. The program is 
* compiled and ran through the Linux servers.
*
* Error messages will occur upon the following situations:
* - Invalid arguments due to incorrect number of parameters
* - Invalid action code
* - Files could not be located
*
* You must have the files called CS6360Asg5TestDataA.txt.txt and
* CS6360Asg5TestDataB.txt in the same directory as the program.
* These are the record files to be read through the program
*
* Commands:
* To create a file:
*	./ProgramName -create textfile.txt data.idx keyLength
*		where:	ProgramName		is the name compiled through Linux
*				-create			is the create command code
*				textfile.txt	is the record text file to be read
*				data.idx		is the index binary file to be created
*				keyLength		is the length of the key
*
* To list the records:
*	./ProgramName -list data.idx startingKey count
*		where:	ProgramName		is the name compiled through Linux
*				-list			is the list command code
*				data.idx		is the index binary file to be created
*				startingKey		is the starting key to be searched
*				count			is the desired number of records to be listed
*
* To find a record:
*	./ProgramName -find data.idx key
*		where:	ProgramName		is the name compiled through Linux
*				-find			is the find command code
*				data.idx		is the index binary file to be created
*				key				is the key to be searched
*
* To find a record:
*	./ProgramName -insert data.idx "Key Data"
*		where:	ProgramName		is the name compiled through Linux
*				-insert			is the insert command code
*				data.idx		is the index binary file to be created
*				"Key Data"		is the record to be inserted
*
* Written by Gary Chen (gxc097020) at The University of Texas at Dallas
* November 19, 2018
******************************************************************************/

#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <bitset>
#include <algorithm>
#include <cstring>
#include <string>
#include <unistd.h>

using namespace std;

struct Metadata
{
	char fileName[256];
	size_t keyLength = 0;
	size_t root = 0;
	size_t maxNode = 0;
	size_t level = 0;
};

Metadata metadata;

struct Record
{
	char key[40];
	string stringKey;
	size_t offset;
};

struct Block
{
	char block[1024];
	size_t numEntry;
};

Block internalKeyBlock;

char nullKey[40] = { 'N','U','L','L' };
char nullcmp[4] = { 'N','U','L','L' };

size_t nullOffset = 0;
size_t pointerHolder;

int createBPTreeIndex(fstream &output, Record *data);
void storeToStruct(Record *data, string line, size_t offset_count, size_t keyLength);
int insertRecord(size_t offsetPtr, fstream &output, Record *data, size_t count, size_t option);
size_t searchBPTreeIndexOffset(size_t offsetPtr, fstream &output, Record *data, size_t level, size_t count, size_t levelCount, size_t option);
void splitNode(char block1[], size_t offsetPtr, fstream &output, Record *data, size_t count, size_t option);
void addNewNodeAfterSplit(char splitBlock2[], char splitBlock3[], fstream &output, size_t offsetPtr, size_t offsetPtr2);
size_t listRecordUsingIndex(size_t offsetPtr, fstream& indexFile, string startingKey, size_t count);
size_t findRecordUsingIndex(size_t searchPtr, fstream& indexFile, string targetKey);
bool icompare_pred(unsigned char a, unsigned char b);
bool icompare(std::string const& a, std::string const& b);

/**************************************************************************
 * Driver function to run the program. Takes arguments from the user via
 * command line.
 **************************************************************************/
int main(int argc, char** argv) {
	string code;
	string fileOneName;
	string fileTwoName;
	string line;
	int keySize;
	int num_arg;

	//Filling the NULL key array with 0s. The amount written will be specified by the metadata.keylength
	for (int i = 4; i < 40; i++)
	{
		nullKey[i] = '0';
	}

	code = argv[1];

	if (argc == 5)
	{
		if (icompare(code, "-create"))
		{
			ifstream fileOne;
			fstream fileTwo;

			fileOneName = argv[2];
			fileTwoName = argv[3];
			keySize = atoi(argv[4]);

			fileOne.open(fileOneName.c_str(), ios::in | ios::binary);
			fileTwo.open(fileTwoName.c_str(), fstream::out | fstream::binary);
			if (access(fileOneName.c_str(), F_OK) == -1)
			{
				cout << endl;
				cout << "Error: Unable to locate file. Please enter valid file name..." << endl;
				cout << endl;
				return 0;
			}
			char *metaBlock = new char[1024];

			//Initialize Metadata
			strcpy(metadata.fileName, argv[2]);

			for (int i = strlen(metadata.fileName); i < 256; i++)
			{
				metadata.fileName[i] = '.';
			}

			metadata.keyLength = keySize;
			metadata.maxNode = (1024 - 8) / (metadata.keyLength + 8);
			strcpy(&metaBlock[0], metadata.fileName);
			memcpy(&metaBlock[256], (char*)&metadata.keyLength, 8);
			memcpy(&metaBlock[264], (char*)&metadata.root, 8);
			memcpy(&metaBlock[272], (char*)&metadata.maxNode, 8);
			memcpy(&metaBlock[280], (char*)&metadata.level, 8);

			fileTwo.seekp(0, ios::beg);
			fileTwo.write(metaBlock, 1024);

			fileTwo.close();

			delete[] metaBlock;

			fileTwo.open(fileTwoName.c_str(), fstream::in | fstream::out | fstream::binary);

			// Create index
			size_t offset_count = 0;

			while (getline(fileOne, line))
			{
				Record *data = new Record;

				storeToStruct(data, line, offset_count, keySize);
				createBPTreeIndex(fileTwo, data);

				delete data;

				offset_count = offset_count + line.length() + 1;
			}

			fileOne.close();

			cout << endl;
			cout << "Index successfully created." << endl;
			cout << endl;

			fileOne.close();

			return 0;
		}
		if (icompare(code, "-list"))
		{
			fstream fileOne;
			string startingKey;
			size_t count;

			char starKey[40];

			fileOneName = argv[2];
			startingKey = argv[3];
			count = atoi(argv[4]);

			fileOne.open(fileOneName.c_str(), ios::in | ios::binary);
			if (access(fileOneName.c_str(), F_OK) == -1)
			{
				cout << endl;
				cout << "Error: Unable to locate file. Please enter valid file name..." << endl;
				cout << endl;
				return 0;
			}

			//Read in the metadablock and retrieve index information
			fileOne.seekg(0, ios::beg);
			fileOne.read(metadata.fileName, 256);
			fileOne.read((char*)&metadata.keyLength, 8);
			fileOne.read((char*)&metadata.root, 8);
			fileOne.read((char*)&metadata.maxNode, 8);
			fileOne.read((char*)&metadata.level, 8);

			//Get the offset pointer to the leaf node (due to way index is structured, 
			// we can always assume the first leaf block starts at 1024)

			// List contents using index
			cout << endl;
			listRecordUsingIndex(1024, fileOne, startingKey, count);
			cout << endl;

			fileOne.close();

			return 0;
		}
	}
	else if (argc == 4)
	{
		if (icompare(code, "-find"))
		{
			fstream fileOne;
			string targetKey;

			char starKey[40];

			fileOneName = argv[2];
			targetKey = argv[3];

			fileOne.open(fileOneName.c_str(), ios::in | ios::binary);
			if (access(fileOneName.c_str(), F_OK) == -1)
			{
				cout << endl;
				cout << "Error: Unable to locate file. Please enter valid file name..." << endl;
				cout << endl;
				return 0;
			}

			//Read in the metadablock and retrieve index information
			fileOne.seekg(0, ios::beg);
			fileOne.read(metadata.fileName, 256);
			fileOne.read((char*)&metadata.keyLength, 8);
			fileOne.read((char*)&metadata.root, 8);
			fileOne.read((char*)&metadata.maxNode, 8);
			fileOne.read((char*)&metadata.level, 8);

			//Get the offset pointer to the leaf node (due to way index is structured, 
			// we can always assume the first leaf block starts at 1024)
			// List contents using index
			cout << endl;
			findRecordUsingIndex(metadata.root, fileOne, targetKey);

			//listRecordUsingIndex(1024, fileOne, startingKey, count, metadata);
			cout << endl;
		}
		if (icompare(code, "-insert"))
		{
			fstream indexFile;
			
			string record;

			char starKey[40];

			fileOneName = argv[2];
			record = argv[3];
			string recordString(record);

			indexFile.open(fileOneName.c_str(), ios::in | ios::out | ios::binary);
			if (access(fileOneName.c_str(), F_OK) == -1)
			{
				cout << endl;
				cout << "Error: Unable to locate file. Please enter valid file name..." << endl;
				cout << endl;
				return 0;
			}

			//Read in the metadablock and retrieve index information
			indexFile.seekg(0, ios::beg);
			indexFile.read(metadata.fileName, 256);
			indexFile.read((char*)&metadata.keyLength, 8);
			indexFile.read((char*)&metadata.root, 8);
			indexFile.read((char*)&metadata.maxNode, 8);
			indexFile.read((char*)&metadata.level, 8);

			fstream recordFile;

			size_t fileNameSize = 0;

			for (int i = 0; i < 256; i++)			//Get length of file name
			{
				if (metadata.fileName[i] != '.')
					fileNameSize++;
			}

			string recordFileName(metadata.fileName, fileNameSize + 1);
			recordFile.open(recordFileName.c_str(), ios::in | ios::out | ios::binary);

			recordFile.seekg(0, ios::end);					//First two lines determines the length (offset pointer)
			size_t offsetEnd = recordFile.tellg();
			recordFile.seekg(0, ios::beg);

			//Store into struct
			Record *entry = new Record;
			memcpy(&entry->key[0], argv[3], metadata.keyLength);
			entry->offset = offsetEnd;

			//Recursive find the leaf node block. Returns the byte pointer to the leaf node block
			size_t offsetPtr;
			offsetPtr = searchBPTreeIndexOffset(metadata.root, indexFile, entry, metadata.level, 0, 1, 1);

			//Count the number of record in the leaf node block
			size_t count = 0;
			char *buffer = new char[1024];
			char *keyBuff = new char[40];
			char *checkNULL = new char[4];
			size_t level = 0;

			indexFile.seekg(0, ios::beg);
			indexFile.seekg(offsetPtr, ios::beg);		//Go to leaf node block
			indexFile.read(buffer, 1024);				// Write the entire block to char array for modification

			int pos = 0;

			char *nullcmp = new char[4];
			strncpy(&nullcmp[0], nullKey, 4);

			while (count <= metadata.maxNode)
			{
				strncpy(keyBuff, buffer + pos, metadata.keyLength);
				strncpy(checkNULL, keyBuff, 4);

				if (checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3])
					break;
				else
					count++;

				pos = pos + metadata.keyLength + 8;
			}

			delete[] keyBuff;
			delete[] buffer;
			char nl[1] = { '\n' };

			if (insertRecord(offsetPtr, indexFile, entry, count, 3) == 0)
			{
				cout << endl;
				cout << "Record successfully inserted." << endl;
				cout << endl;

				recordFile.seekg(offsetEnd, ios::beg);
				recordFile.write(record.c_str(), strlen(recordString.c_str()));
				recordFile.write(nl, 1);
			}

			return 0;
		}
	}

	else if (!icompare(code, "-create") && !icompare(code, "-list") && !icompare(code, "-find") && !icompare(code, "-insert"))
	{
		cout << endl;
		cout << "Error: Invalid code. Valid codes are -c or -l. Please enter a valid code..." << endl;
		cout << endl;
		return 0;
	}
}

/**************************************************************************
 * Function to store records to struct
 **************************************************************************/
void storeToStruct(Record *data, string line, size_t offset_count, size_t keyLength)
{
	string stringKey;
	int lineCount = 0;

	stringKey = line.substr(0, keyLength);
	strcpy(data->key, stringKey.c_str());
	data->stringKey = stringKey;
	data->offset = offset_count;
}

/**************************************************************************
 * Function to store records to struct
 **************************************************************************/
int createBPTreeIndex(fstream &output, Record *data)
{
	//Update Metadata to point to first root node block
	if (metadata.root == 0)
	{
		char *metaBlock = new char[1024];

		metadata.root = 1024;
		metadata.level++;

		memcpy(&metaBlock[0], metadata.fileName, 256);
		memcpy(&metaBlock[256], (char*)&metadata.keyLength, 8);
		memcpy(&metaBlock[264], (char*)&metadata.root, 8);
		memcpy(&metaBlock[272], (char*)&metadata.maxNode, 8);
		memcpy(&metaBlock[280], (char*)&metadata.level, 8);

		output.seekp(0, ios::beg);
		output.write(metaBlock, 1024);

		delete[] metaBlock;

		char *block1 = new char[1024];

		//Write the block1 to file
		memcpy(&block1[0], data->key, metadata.keyLength);
		memcpy(&block1[metadata.keyLength], (char*)&data->offset, 8);
		memcpy(&block1[metadata.keyLength + 8], nullKey, metadata.keyLength);
		memcpy(&block1[2*metadata.keyLength + 8], (char*)&nullOffset, 8);

		output.seekp(1024, ios::beg);
		output.write(block1, 1024);

		delete[] block1;
	}
	else if(metadata.root > 0)
	{
		//Conduct B+ Tree search and insert

		//Recursive find the leaf node block. Returns the byte pointer to the leaf node block
		size_t offsetPtr;
		offsetPtr = searchBPTreeIndexOffset(metadata.root, output, data, metadata.level, 0, 1, 1);

		//Count the number of record in the leaf node block
		size_t count = 0;
		char *buffer = new char[1024];
		char *keyBuff = new char[40];
		char *checkNULL = new char[4];
		size_t level = 0;
		
		output.seekg(0, ios::beg);
		output.seekg(offsetPtr, ios::beg);		//Go to leaf node block
		output.read(buffer, 1024);				// Write the eniter block to char array for modification

		int pos = 0;

		char *nullcmp = new char[4];
		strncpy(&nullcmp[0], nullKey, 4);

		while (count <= metadata.maxNode)
		{
			strncpy(keyBuff, buffer + pos, metadata.keyLength);
			strncpy(checkNULL, keyBuff, 4);

			if(checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3])
				break;
			else
				count++;
				
			pos = pos + metadata.keyLength + 8;
		}

		delete[] keyBuff;
		delete[] buffer;

		insertRecord(offsetPtr, output, data, count, 1);
	}
	
}

/**************************************************************************
* Function to search for the correct offset pointer in the B+ Tree Index
**************************************************************************/
size_t searchBPTreeIndexOffset(size_t offsetPtr, fstream &output, Record *data, size_t level, size_t count, size_t levelCount, size_t option)
{
	size_t target = 0;
	output.seekg(offsetPtr, ios::beg);

	if (option == 1)	
		target = level;				//Find the target leaf node
	else if (option == 2)
		target = level - 1;			//Find the target internal node

	if (levelCount < target)		//if block is an internal node
	{
		//Get key to compare
		char *keyBuff = new char[40];
		char *cmpNULL = new char[4];

		//Read in a key
		output.seekg(offsetPtr+8, ios::beg);		//internal node starts with an 8 byte key
		output.read(keyBuff, metadata.keyLength);
		memcpy(&cmpNULL[0], keyBuff, metadata.keyLength);

		//Compare keys
		if ((keyBuff[0] == nullcmp[0] && keyBuff[1] == nullcmp[1] && keyBuff[2] == nullcmp[2] && keyBuff[3] == nullcmp[3]) || strcmp(data->key, keyBuff) < 0)
		{
			output.seekg(offsetPtr, ios::beg);		//read the left offset
			output.read((char*)&offsetPtr, 8);		
			levelCount++;							//next level in the tree
			count = 0;								//reset count
			
			delete[] keyBuff;
			delete[] cmpNULL;
			
			return searchBPTreeIndexOffset(offsetPtr, output, data, level, count, levelCount, option);
		}
		else if (strcmp(data->key, keyBuff) > 0)
		{
			if(count < metadata.maxNode)	//stay in same level and return the next key
			{
				offsetPtr = offsetPtr + metadata.keyLength + 8;
				count++;
				
				delete[] keyBuff;
				delete[] cmpNULL;
				return searchBPTreeIndexOffset(offsetPtr, output, data, level, count, levelCount, option);
			}
			else if (count == metadata.maxNode) //Will probably never happen?
			{
				offsetPtr = offsetPtr + metadata.keyLength + 8;
				levelCount++;								//next level in the tree
				count = 0;								//reset count

				delete[] keyBuff;
				delete[] cmpNULL;
				return searchBPTreeIndexOffset(offsetPtr, output, data, level, count, levelCount, option);
			}
		}
	}

	return offsetPtr;
}

/**************************************************************************
* Function to insert records into B+ Tree index
**************************************************************************/
int insertRecord(size_t offsetPtr, fstream &output, Record *data, size_t count, size_t option)
{
	size_t offsetStart = 0;

	if (option == 1 || option == 3)
		offsetStart = 0;
	else if (option == 2)
		offsetStart = 8;

	char *block1 = new char[1024];
	char *keyBuff = new char[40];
	char *checkNULL = new char[4];

	output.seekg(offsetPtr, ios::beg);
	output.read(block1, 1024);

	size_t offsetPtr2;
	output.seekg(offsetPtr + offsetStart + (metadata.keyLength + 8)*(count) + metadata.keyLength, ios::beg);		//Keep track of the pointer value to the next leaf or the NULL offset
	output.read((char*)&offsetPtr2, 8);
	pointerHolder = offsetPtr2;

	int numRec = 0;
	while (numRec <= count)
	{

		output.seekg(offsetPtr + offsetStart + (metadata.keyLength + 8)*numRec, ios::beg);		//Go to the next key in leaf node	
		output.read(&keyBuff[0], metadata.keyLength);

		memcpy(&checkNULL[0], keyBuff, metadata.keyLength);

		string keyString(data->key);
		string keyBuffString(keyBuff);

		if ((checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3]) || strcmp(data->key, keyBuff) < 0)		//If less than, insert into position and shift others to the right
		{

			char *block2 = new char[1024];	//Used for holding shifting keys
			output.seekg(offsetPtr + offsetStart + (metadata.keyLength + 8)*numRec, ios::beg);
			output.read(block2, (metadata.keyLength + 8)*(count - numRec));	//Copy the keys to be shifted to the right, excluding NULL key

			memcpy(&block1[offsetStart + (metadata.keyLength + 8)*numRec], data->key, metadata.keyLength); // write the key/offset to be inserted into the correct position
			memcpy(&block1[offsetStart + (metadata.keyLength + 8)*numRec + metadata.keyLength], (char*)&data->offset, 8);

			memcpy(&block1[offsetStart + (metadata.keyLength + 8)*numRec + metadata.keyLength + 8], block2, (metadata.keyLength + 8)*(count - numRec)); //Copy the shifted keys back into buffer block

			delete[] block2;

			if ((count + 1) < metadata.maxNode)	//If total # of nodes after insertion is still less than max capacity, add the NULL key and NULL offset
			{
				memcpy(&block1[offsetStart + (metadata.keyLength + 8)*(count + 1)], nullKey, metadata.keyLength);
				memcpy(&block1[offsetStart + (metadata.keyLength + 8)*(count + 1) + metadata.keyLength], (char*)&offsetPtr2, 8);

				//write block to file
				output.seekg(offsetPtr, ios::beg);
				output.write(block1, 1024);
				delete[] block1;
				return 0;
			}
			else if ((count + 1) == metadata.maxNode)
			{
				splitNode(block1, offsetPtr, output, data, count + 1, option);
				return 0;
			}
		}
		else if (strcmp(data->key, keyBuff) > 0)		//If greater than, move to next pair
		{
			if (numRec < count)		//If greater than but not reached NULL key, keep traversing
			{
				numRec++;
				//offsetPtr = offsetPtr + metadata.keyLength + 8;
			}
		}
		else if (icompare(keyString, keyBuffString))
			if (option == 3)
			{
				cout << endl;
				cout << "A record with that key already exits." << endl;
				cout << endl;
				return 1;
			}
			else
			{
				cout << endl;
				cout << "Duplicate Found." << endl;
				cout << endl;
				return 0;
			}
	}
}

/**************************************************************************
* Function to split node
**************************************************************************/
void splitNode(char block1[], size_t offsetPtr, fstream &output, Record *data, size_t count, size_t option)
{
	size_t offsetStart = 0;
	if (option == 1)
		offsetStart = 0;
	else if (option == 2)
		offsetStart = 8;

	// CONDUCT NODE SPIT
	char *splitBlock2 = new char[1024];		//used for holding the second half of block1
	char *splitBlock3 = new char[1024];		//used for holding the internal nodes

	/* Create a copy leaf block, block2 */
	memcpy(&splitBlock2[0], &block1[offsetStart + (metadata.keyLength + 8)*((count + 1) / 2)], (metadata.keyLength + 8)*((count + 1) / 2));	//copy the last half of block 1 to block 2
	memcpy(&splitBlock2[offsetStart + (metadata.keyLength + 8)*((count + 1) / 2)], nullKey, metadata.keyLength);								//add the null key to the end of block 2
	memcpy(&splitBlock2[offsetStart + (metadata.keyLength + 8)*((count + 1) / 2) + metadata.keyLength], (char*)&pointerHolder, 8);				//add the null offset to the end of block 2

	for (int i = (metadata.keyLength + 8)*((count + 1) / 2) + metadata.keyLength + 8; i < 1024; i++)
		splitBlock2[i] = ' ';

	output.seekg(0, ios::end);					//First two lines determines the length (offset pointer)
	size_t offsetPtr2 = output.tellg(); 		
	output.seekg(offsetPtr2, ios::beg);			//Seek to a offsetPtr2
	output.write(splitBlock2, 1024);					//Append block2 to index

	/* Modify block1*/
	for (int i = (offsetStart + (metadata.keyLength + 8)*((count + 1) / 2)); i < 1024; i++)
		block1[i] = ' ';	//Remove the last half of block 1

	memcpy(&block1[offsetStart + (metadata.keyLength + 8)*((count + 1) / 2)], nullKey, metadata.keyLength);								//add the null key to the end of block 1
	memcpy(&block1[offsetStart + (metadata.keyLength + 8)*((count + 1) / 2) + metadata.keyLength], (char*)&offsetPtr2, 8);				//add the offsetPtr2 to the end of block 1

	output.seekg(offsetPtr, ios::beg);			//Seek to a offsetPtr
	output.write(block1, 1024);					//Rewrite block1 to index

	delete[] block1;

	/* Create internal node block, Block3 */
	if (metadata.level < 2)	//No internal node exist yet
	{
		addNewNodeAfterSplit(splitBlock2, splitBlock3, output, offsetPtr, offsetPtr2);
	}

	else if (metadata.level >= 2)	//There exist an internal node
	{
		size_t offsetIntern = 0;
		offsetIntern = searchBPTreeIndexOffset(metadata.root, output, data, metadata.level, 0, 1, 2);

		//Count the number of record in the internal node block
		size_t count = 0;
		char *buffer = new char[1024];
		char *keyBuff = new char[40];
		char *checkNULL = new char[4];
		size_t level = 0;

		output.seekg(0, ios::beg);
		output.seekg(offsetIntern, ios::beg);		//Go to internal node block
		output.read(buffer, 1024);					// Write the entire block to char array for modification

		int pos = 8;

		char *nullcmp = new char[4];
		strncpy(&nullcmp[0], nullKey, 4);

		while (count <= metadata.maxNode)
		{
			strncpy(keyBuff, buffer + pos, metadata.keyLength);
			strncpy(checkNULL, keyBuff, 4);

			if (keyBuff[0] == nullcmp[0] && keyBuff[1] == nullcmp[1] && keyBuff[2] == nullcmp[2] && keyBuff[3] == nullcmp[3])
				break;
			else
				count++;

			pos = pos + metadata.keyLength + 8;
		}
		
		delete[] keyBuff;
		delete[] buffer;

		Record *internalData = new Record;
		memcpy(&internalData->key[0], &splitBlock2[0], metadata.keyLength);
		internalData->offset = offsetPtr2;
		
		insertRecord(offsetIntern, output, internalData, count, 2);
		
		//delete[] internalData;
	}
}

/**************************************************************************
* Function to add a new node to B+ tree index
**************************************************************************/
void addNewNodeAfterSplit(char splitBlock2[], char splitBlock3[], fstream &output, size_t offsetPtr, size_t offsetPtr2)
{

	memcpy(&splitBlock3[0], (char*)&offsetPtr, 8);											//Copy the pointer to the left leaf node
	memcpy(&splitBlock3[8], &splitBlock2[0], metadata.keyLength);							//Copy the first key from the right leaf node
	memcpy(&splitBlock3[8 + metadata.keyLength], (char*)&offsetPtr2, 8);					//Copy the pointer to the right leaf node
	memcpy(&splitBlock3[metadata.keyLength + 8 * 2], nullKey, metadata.keyLength);			//add the null key to the end of block 3
	memcpy(&splitBlock3[metadata.keyLength * 2 + 8 * 2], (char*)&nullOffset, 8);				//add the null offset to the end of block 3

	delete[] splitBlock2;

	output.seekg(0, ios::end);					//First two lines determines the length (offset pointer)
	size_t offsetIntern = output.tellg();
	output.seekg(offsetIntern, ios::beg);			//Seek to a offsetIntern
	output.write(splitBlock3, 1024);					//Append block3 to index

	delete[] splitBlock3;

	//update metadata block for the root
	char *metaBlock = new char[1024];
	metadata.root = offsetIntern;
	metadata.level = metadata.level + 1;
	strcpy(&metaBlock[0], metadata.fileName);
	memcpy(&metaBlock[256], (char*)&metadata.keyLength, 8);
	memcpy(&metaBlock[264], (char*)&metadata.root, 8);
	memcpy(&metaBlock[272], (char*)&metadata.maxNode, 8);
	memcpy(&metaBlock[280], (char*)&metadata.level, 8);

	output.seekg(0, ios::beg);
	output.write(metaBlock, 1024);

	delete[] metaBlock;
}

/**************************************************************************
* Function to list records
**************************************************************************/
size_t listRecordUsingIndex(size_t offsetPtr, fstream& indexFile, string startingKey, size_t count)
{
	/* Get Metadata information */
	ifstream recordFile;
	size_t stringSize;

	size_t fileNameSize = 0;

	for (int i = 0; i < 256; i++)			//Get length of file name
	{
		if (metadata.fileName[i] != '.')
			fileNameSize++;
	}

	string recordFileName(metadata.fileName, fileNameSize+1);
	char *keyBuff = new char[40];

	Record *entry = new Record;

	strncpy(&entry->key[0], startingKey.c_str(), metadata.keyLength);


	strncpy(entry->key, startingKey.c_str(), sizeof(stringSize));

	recordFile.open(recordFileName.c_str(), ios::in | ios::binary);

	//Search for the leaf node that the entry should be in
	offsetPtr = searchBPTreeIndexOffset(metadata.root, indexFile, entry, metadata.level, 0, 1, 1);

	char *checkNULL = new char[40];

	size_t stopCount = 0;
	size_t traverseCount = 0;
	size_t offset;

	int numRec = 0;
	while (stopCount != 1)
	{

		indexFile.seekg(offsetPtr + (metadata.keyLength + 8)*numRec, ios::beg);		//Go to the next key in leaf node	
		indexFile.read(&keyBuff[0], metadata.keyLength);

		memcpy(&checkNULL[0], keyBuff, metadata.keyLength);

		string keyString(entry->key);
		string keyBuffString(keyBuff);

		if ((strcmp(keyString.c_str(), keyBuffString.c_str()) < 0))		//If less than, insert into position and shift others to the right
		{
			cout << "Entry not found. Displaying the next " << count << " records greater than entry, or up to the last record in the list:" << endl << endl;
			while (traverseCount < count)		//Print the next 10 keys
			{
				indexFile.seekg(offsetPtr + (8 + metadata.keyLength)*numRec, ios::beg);
				indexFile.read(&checkNULL[0], metadata.keyLength);

				indexFile.seekg(offsetPtr + metadata.keyLength + (8 + metadata.keyLength)*numRec, ios::beg);
				indexFile.read((char*)&offset, 8);

				if (checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3] && offset != 0)
				{
					offsetPtr = offset;

					indexFile.seekg(offsetPtr, ios::beg);
					indexFile.read(&checkNULL[0], metadata.keyLength);

					indexFile.seekg(offsetPtr + metadata.keyLength, ios::beg);
					indexFile.read((char*)&offset, 8);

					numRec = 0;
				}
				else if (checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3] && offset == 0)
					break;

				string recLine;
				recordFile.seekg(offset, ios::beg);
				getline(recordFile, recLine);
				cout << recLine << endl;
				traverseCount++;
				numRec++;
			}
			stopCount = 1;
		}

		if ((strcmp(keyString.c_str(), keyBuffString.c_str()) > 0))		//Move to the next record in the same node
		{
			numRec++;
		}
		if (((strcmp(keyString.c_str(), keyBuffString.c_str()) == 0)))		//If reaches NULL of a leaf
		{
			cout << "Entry found. Displaying " << count << " records starting with entry, or up to the last record in the list:" << endl << endl;
			while (traverseCount < count)		//Print the next 10 keys
			{
				indexFile.seekg(offsetPtr + (8 + metadata.keyLength)*numRec, ios::beg);
				indexFile.read(&checkNULL[0], metadata.keyLength);

				indexFile.seekg(offsetPtr + metadata.keyLength + (8 + metadata.keyLength)*numRec, ios::beg);
				indexFile.read((char*)&offset, 8);

				if (checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3] && offset != 0)
				{
					offsetPtr = offset;

					indexFile.seekg(offsetPtr, ios::beg);
					indexFile.read(&checkNULL[0], metadata.keyLength);

					indexFile.seekg(offsetPtr + metadata.keyLength, ios::beg);
					indexFile.read((char*)&offset, 8);

					numRec = 0;
				}
				else if (checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3] && offset == 0)
					break;

				string recLine;
				recordFile.seekg(offset, ios::beg);
				getline(recordFile, recLine);
				cout << recLine << endl;
				traverseCount++;
				numRec++;
			}
			stopCount = 1;
		}
	}
}

/**************************************************************************
* Function to find a specific record
**************************************************************************/
size_t findRecordUsingIndex(size_t searchPtr, fstream& indexFile, string startingKey)
{
	/* Get Metadata information */
	ifstream recordFile;
	size_t stringSize;

	size_t fileNameSize = 0;

	for (int i = 0; i < 256; i++)			//Get length of file name
	{
		if (metadata.fileName[i] != '.')
			fileNameSize++;
	}

	string recordFileName(metadata.fileName, fileNameSize + 1);
	char *keyBuff = new char[40];

	Record *entry = new Record;

	strncpy(&entry->key[0], startingKey.c_str(), metadata.keyLength);


	strncpy(entry->key, startingKey.c_str(), sizeof(stringSize));

	recordFile.open(recordFileName.c_str(), ios::in | ios::binary);

	//Search for the leaf node that the entry should be in
	searchPtr = searchBPTreeIndexOffset(metadata.root, indexFile, entry, metadata.level, 0, 1, 1);

	char *checkNULL = new char[40];

	size_t stopCount = 0;
	size_t traverseCount = 0;
	size_t offset;

	int numRec = 0;
	while (stopCount != 1)
	{

		indexFile.seekg(searchPtr + (metadata.keyLength + 8)*numRec, ios::beg);		//Go to the next key in leaf node	
		indexFile.read(&keyBuff[0], metadata.keyLength);

		memcpy(&checkNULL[0], keyBuff, metadata.keyLength);

		string keyString(entry->key);
		string keyBuffString(keyBuff);

		if (checkNULL[0] == nullcmp[0] && checkNULL[1] == nullcmp[1] && checkNULL[2] == nullcmp[2] && checkNULL[3] == nullcmp[3])
		{
			searchPtr = offset;

			indexFile.seekg(searchPtr, ios::beg);
			indexFile.read(&checkNULL[0], metadata.keyLength);

			indexFile.seekg(searchPtr + metadata.keyLength, ios::beg);
			indexFile.read((char*)&offset, 8);

			numRec = 0;
		}

		if ((strcmp(keyString.c_str(), keyBuffString.c_str()) < 0))		//If less than, insert into position and shift others to the right
		{
			cout << "Could not find record." << endl;
			stopCount = 1;
		}

		if ((strcmp(keyString.c_str(), keyBuffString.c_str()) > 0))		//Move to the next record in the same node
		{
			numRec++;
		}
		if (((strcmp(keyString.c_str(), keyBuffString.c_str()) == 0)))		//If reaches NULL of a leaf
		{
			indexFile.seekg(searchPtr + (8 + metadata.keyLength)*numRec, ios::beg);
			indexFile.read(&checkNULL[0], metadata.keyLength);

			indexFile.seekg(searchPtr + metadata.keyLength + (8 + metadata.keyLength)*numRec, ios::beg);
			indexFile.read((char*)&offset, 8);

			cout << "At " << offset << ", record: ";

			string recLine;
			recordFile.seekg(offset, ios::beg);
			getline(recordFile, recLine);
			cout << recLine << endl;
			traverseCount++;
			numRec++;

			stopCount = 1;
		}
	}
}

/**************************************************************************
* Utility functions
**************************************************************************/
bool icompare_pred(unsigned char a, unsigned char b)
{
	return tolower(a) == tolower(b);
}

bool icompare(std::string const& a, std::string const& b)
{
	if (a.length() == b.length()) {
		return std::equal(b.begin(), b.end(),
			a.begin(), icompare_pred);
	}
	else {
		return false;
	}
}