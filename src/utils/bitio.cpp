/* ------------------------------------------------------------------------

    Bitio.cpp

    Used to open, read, write compressed conversation files.

    STANDALONE application.

    Version 0.01

   ------------------------------------------------------------------------ */


#include <stdio.h>
#include <stdlib.h>
#include "bitio.h"


#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.hpp"
#endif

/* Add for the buffer io */
#define PACIFIER_COUNT	2047

BIT_FILE *OpenOutputBitFile( char *name )
{
    BIT_FILE *bit_file;

//	bit_file = (BIT_FILE *) malloc( sizeof( BIT_FILE ) );
    bit_file = new BIT_FILE;
    if ( bit_file == NULL )
        return( bit_file );
    bit_file->file = fopen( name, "wb" );
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;
    return( bit_file );
}

BIT_FILE *OpenInputBitFile( char *name )
{
    BIT_FILE *bit_file;

//	bit_file = (BIT_FILE *) malloc( sizeof( BIT_FILE ) );
    bit_file = new BIT_FILE;
    if ( bit_file == NULL )
	return( bit_file );
    bit_file->file = fopen( name, "rb" );
    bit_file->rack = 0;
    bit_file->mask = 0x80;
    bit_file->pacifier_counter = 0;
    return( bit_file );
}

void CloseOutputBitFile( BIT_FILE *bit_file )
{
    if ( bit_file->mask != 0x80 )
        if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
			printf( "Fatal error in CloseBitFile!" );
    fclose( bit_file->file );
    delete [] bit_file;
}

void CloseInputBitFile( BIT_FILE *bit_file )
{
    fclose( bit_file->file );
    delete [] bit_file;
}

void OutputBit( BIT_FILE *bit_file, int bit )
{
    if ( bit )
        bit_file->rack |= bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 ) {
	if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
		printf( "Fatal error in OutputBit!" );
	else
        if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
		putc( '.', stdout );
	bit_file->rack = 0;
	bit_file->mask = 0x80;
    }
}

void OutputBits( BIT_FILE *bit_file, unsigned long code, int count )
	{
    unsigned long mask;

    mask = 1L << ( count - 1 );
    while ( mask != 0) 
		{
        if ( mask & code )
            bit_file->rack |= bit_file->mask;
        bit_file->mask >>= 1;
        if ( bit_file->mask == 0 ) 
			{
			if ( putc( bit_file->rack, bit_file->file ) != bit_file->rack )
				printf( "Fatal error in OutputBit!" );
			else if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
				putc( '.', stdout );

			bit_file->rack = 0;
            bit_file->mask = 0x80;
			}
        mask >>= 1;
		}
	}

int InputBit( BIT_FILE *bit_file )
{
    int value;
	size_t numRead;

    if ( bit_file->mask == 0x80 ) 
		{
		
		numRead = fread( &bit_file->rack, sizeof( char ), 1, bit_file->file );

    //    bit_file->rack = getc( bit_file->file );
        if ( bit_file->rack == EOF )
			return (unsigned long)EOF;

		if ( ( bit_file->pacifier_counter++ & PACIFIER_COUNT ) == 0 )
			putc( '.', stdout );
		}
    value = bit_file->rack & bit_file->mask;
    bit_file->mask >>= 1;
    if ( bit_file->mask == 0 )
		bit_file->mask = 0x80;
    return( value ? 1 : 0 );
}

unsigned long InputBits( BIT_FILE *bit_file, int bit_count )
{
    unsigned long mask;
    unsigned long return_value;
	size_t	numRead;

    mask = 1L << ( bit_count - 1 );
    return_value = 0;
    while ( mask != 0) 
		{ 
		/*
		 * 0x80 is silence in sound files, clear or zero
		 * mask is cleared to 0x80 when mask for last bit read is 0
		 * then you read in new byte and set rack
		 */
		if ( bit_file->mask == 0x80 ) 
			{
			
			numRead = fread( &bit_file->rack, sizeof( char ), 1, bit_file->file );
		//	bit_file->rack = getc( bit_file->file );
			if ( bit_file->rack == EOF )
				return (unsigned long)EOF;
			}
		if ( bit_file->rack & bit_file->mask )
				return_value |= mask;
		
		/*
		 * Packs a byte and a half into one byte. A byte for me is 5 bits - On Compression.
		 * This is reversed for expansion.
		 */
		mask >>= 1;	
		bit_file->mask >>= 1;
		if ( bit_file->mask == 0 )
			bit_file->mask = 0x80;
		}
    return( return_value );
}

void FilePrintBinary( FILE *file, unsigned int code, int bits )
{
    unsigned int mask;

    mask = 1 << ( bits - 1 );
    while ( mask != 0 ) {
        if ( code & mask )
            fputc( '1', file );
        else
            fputc( '0', file );
        mask >>= 1;
    }
}

/*************************** End of BITIO.C **************************/
