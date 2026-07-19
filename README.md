# Link9TN9

An segment-aware cross-linker to generate TI-99/4A memory-image files on your PC  

## Features

* Handles segments (code, data or program), flexible loading strategy
* Custom-defined memory models
* Callable in interactive, command line or script mode
* Accept libraries of concatenated object files (e.g. created with Lib9TN9)
* Loosely based on the syntax of the RAG linker by R.A. Green

## Installation

Download the zip file, which contains the following:
* Link9tn9.exe	The linker itself
* Lib9tn9.exe	A library manager
* Devices.spt	A stock script containing descriptions of most TI-99/4A memory devices

## Usage

Interactively: link9tn9.exe   
Script-based: link9tn9.exe scriptfile  
Command line: link9tn9exe options  

Options can be the following
-l file_list 	Comma-separated list of DF80 files  
-L file_list	Comma-separated list of library files  
-o file_list	Comma-separated list of output files to create  
-m model	Memory model  
-s filename	Process filename as a script  

See manual for detailed commands: "LINK9TN9 user manual.docx". Also available at: https://nouspikel.com/titechpages.htm  

## License

Written in C++ with Visual Studio 4.2
Copyright (c) 2020-2026 Thierry Nouspikel
