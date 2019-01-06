Author: Gary Chen

Follow the steps to run the programs:
1. Extract all the files from the zip folder and use WinSCP application (or other similar software)
   to move the files into the current working directory (home directory will suffice) of the UNIX system.
2. Open a terminal and CD into the current working directory.
3. The run file named 'BPIndex' should already available from the zip. Run the following 
   commands to test the simulation:

   To create a file:
	./ProgramName -create textfile.txt data.idx keyLength
		where:	ProgramName		is the name compiled through Linux
				-create			is the create command code
				textfile.txt	is the record text file to be read
				data.idx		is the index binary file to be created
				keyLength		is the length of the key

  To list the records:
	./ProgramName -list data.idx startingKey count
		where:	ProgramName		is the name compiled through Linux
				-list			is the list command code
				data.idx		is the index binary file to be created
				startingKey		is the starting key to be searched
				count			is the desired number of records to be listed

  To find a record:
	./ProgramName -find data.idx key
		where:	ProgramName		is the name compiled through Linux
				-find			is the find command code
				data.idx		is the index binary file to be created
				key				is the key to be searched

  To find a record:
	./ProgramName -insert data.idx "Key Data"
		where:	ProgramName		is the name compiled through Linux
				-insert			is the insert command code
				data.idx		is the index binary file to be created
				"Key Data"		is the record to be inserted

4. If there is no .out file, or if you want to check to see if it compile correctly, do the following 
   commands:
		
	g++ -std=c++11 -o BPIndex BPIndex.cpp

   Be sure to type in the correct spacings.