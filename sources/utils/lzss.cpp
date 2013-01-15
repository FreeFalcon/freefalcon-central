/*--------------------------------------------------------------------
	$Header: /home/cvsroot/RedCobra/utils/lzss.cpp,v 1.1.1.1 2005/07/28 07:31:54 Red Exp $

	This file lzss.c was originally the bottom of the carman.c
	program, given in the Data Compression Book, by Nelson and Gailly,
	2nd Ed., M&T Books, c.1996.  I have taken out the all of the 
	archive related stuff, and left in the lzss compression.  Any 
	change I make to the file will be marked by an empty comment.

	Changes by Kevin Klemmick on Aug 27, 1997: converted to c++ and
	made MSDV c++ compliant.

	More additions on Aug 29, 1997: Added Jim D's versions.

	Changes by Kevin Klemmick on Aug 13, 1998: Set up for external 
	library - user can define wether to use "C" or "C++" (for linkage
	purposes, primarily).

	Additional changes: Implimented compression contexts, for multi-
	threaded support.

--------------------------------------------------------------------*/

#include <stddef.h>
#include <memory.h>
#ifdef INCLUDE_FILE_COMPRESSION
#include "bitio.cpp"
#endif
#include "lzss.h"

//sfr: added for buffer checking: checks and change size
#define CHSZ(x, sz) ;
//if ((x -= sz) <= 0) return -1;

typedef unsigned char uchar;

// Various constants used to define the compression parameters.  The
// INDEX_BIT_COUNT tells how many bits we allocate to indices into the
// text window.  This directly determines the WINDOW_SIZE.  The
// LENGTH_BIT_COUNT tells how many bits we allocate for the length of
// an encode phrase. This determines the size of the look ahead buffer.
// The TREE_ROOT is a special node in the tree that always points to
// the root node of the binary phrase tree.  END_OF_STREAM is a special
// index used to flag the fact that the file has been completely
// encoded, and there is no more data.  UNUSED is the null index for
// the tree. MOD_WINDOW() is a macro used to perform arithmetic on tree
// indices.
#define INDEX_BIT_COUNT      12
#define LENGTH_BIT_COUNT     4
#define WINDOW_SIZE          ( 1 << INDEX_BIT_COUNT )
#define RAW_LOOK_AHEAD_SIZE  ( 1 << LENGTH_BIT_COUNT )
#define BREAK_EVEN           ( ( 1 + INDEX_BIT_COUNT + LENGTH_BIT_COUNT ) / 9 )
#define LOOK_AHEAD_SIZE      ( RAW_LOOK_AHEAD_SIZE + BREAK_EVEN )
#define TREE_ROOT            WINDOW_SIZE
#define END_OF_STREAM        0
#define UNUSED               0
#define MOD_WINDOW( a )      ( ( a ) & ( WINDOW_SIZE - 1 ) )

// Compression Context (For multi-threaded usaged)
// Basically, these were globals before, now they're allocated by the stack per call
typedef struct  
	{
	// These are the global data structures used in this program.
	// The window[] array is exactly that, the window of previously seen
	// text, as well as the current look ahead text.  The tree[] structure
	// contains the binary tree of all of the strings in the window sorted
	// in order.
 	unsigned char window[ WINDOW_SIZE ];
	struct 
		{
		int parent;
		int smaller_child;
		int larger_child;
		} tree[ WINDOW_SIZE + 1 ];
	unsigned char DataBuffer[ 17 ];
	int FlagBitMask;
	unsigned int BufferOffset;
	unsigned int OldBufferOffset;
	int original_size; 
	int compressed_size;
	int inc_input_string;
	int inc_output_string;
	} LZSS_COMP_CTXT;

/*
 * The second set of compression routines are found here.  These
 * routines implement LZSS compression and expansion using 12 bit
 * index pointers and 4 bit match lengths.  These values were
 * specifically chosen because they allow for "blocked I/O".  Because
 * of their values, we can pack match/length pairs into pairs of
 * bytes, with characters that don't have matches going into single
 * bytes.  This helps increase I/O since single bit input and
 * output does not have to be employed.  Other than this single change,
 * this code is identical to the LZSS code used earlier in the book.
 */

/*
 * Function prototypes
 */

void InitTree( int r, LZSS_COMP_CTXT* ctxt );
void ContractNode( int old_node, int new_node, LZSS_COMP_CTXT* ctxt );
void ReplaceNode( int old_node, int new_node, LZSS_COMP_CTXT* ctxt );
int FindNextNode( int node, LZSS_COMP_CTXT* ctxt );
void DeleteString( int p, LZSS_COMP_CTXT* ctxt );
int AddString( int new_node, int *match_position, LZSS_COMP_CTXT* ctxt );
void InitOutputBuffer( LZSS_COMP_CTXT* ctxt );
int FlushOutputBuffer( uchar *output_string, LZSS_COMP_CTXT* ctxt);
int OutputChar( int data, uchar *output_string, LZSS_COMP_CTXT* ctxt );
int OutputPair( int position, int length, uchar *output_string, LZSS_COMP_CTXT* ctxt );
void InitInputBuffer( uchar *input_string, LZSS_COMP_CTXT* ctxt);
int InputBit( uchar *input_string, LZSS_COMP_CTXT* ctxt);

/*
 * Since the tree is static data, it comes up with every node
 * initialized to 0, which is good, since 0 is the UNUSED code.
 * However, to make the tree really usable, a single phrase has to be
 * added to the tree so it has a root node.  That is done right here.
 */
void InitTree (int r, LZSS_COMP_CTXT* ctxt)
{
    int i;

    for ( i = 0 ; i < ( WINDOW_SIZE + 1 ) ; i++ ) {
        ctxt->tree[ i ].parent = UNUSED;
        ctxt->tree[ i ].larger_child = UNUSED;
        ctxt->tree[ i ].smaller_child = UNUSED;
    }
    ctxt->tree[ TREE_ROOT ].larger_child = r;
    ctxt->tree[ r ].parent = TREE_ROOT;
    ctxt->tree[ r ].larger_child = UNUSED;
    ctxt->tree[ r ].smaller_child = UNUSED;
}

/*
 * This routine is used when a node is being deleted.  The link to
 * its descendant is broken by pulling the descendant in to overlay
 * the existing link.
 */
void ContractNode(int old_node, int new_node, LZSS_COMP_CTXT* ctxt )
{
    ctxt->tree[ new_node ].parent = ctxt->tree[ old_node ].parent;
    if ( ctxt->tree[ ctxt->tree[ old_node ].parent ].larger_child == old_node )
        ctxt->tree[ ctxt->tree[ old_node ].parent ].larger_child = new_node;
    else
        ctxt->tree[ ctxt->tree[ old_node ].parent ].smaller_child = new_node;
    ctxt->tree[ old_node ].parent = UNUSED;
}

/*
 * This routine is also used when a node is being deleted.  However,
 * in this case, it is being replaced by a node that was not previously
 * in the tree.
 */
void ReplaceNode(int old_node, int new_node, LZSS_COMP_CTXT* ctxt )
{
    int parent;

    parent = ctxt->tree[ old_node ].parent;
    if ( ctxt->tree[ parent ].smaller_child == old_node )
        ctxt->tree[ parent ].smaller_child = new_node;
    else
        ctxt->tree[ parent ].larger_child = new_node;
    ctxt->tree[ new_node ] = ctxt->tree[ old_node ];
    ctxt->tree[ ctxt->tree[ new_node ].smaller_child ].parent = new_node;
    ctxt->tree[ ctxt->tree[ new_node ].larger_child ].parent = new_node;
    ctxt->tree[ old_node ].parent = UNUSED;
}

/*
 * This routine is used to find the next smallest node after the node
 * argument.  It assumes that the node has a smaller child.  We find
 * the next smallest child by going to the smaller_child node, then
 * going to the end of the larger_child descendant chain.
*/
int FindNextNode (int node, LZSS_COMP_CTXT* ctxt)
{
    int next;

    next = ctxt->tree[ node ].smaller_child;
    while ( ctxt->tree[ next ].larger_child != UNUSED )
        next = ctxt->tree[ next ].larger_child;
    return( next );
}

/*
 * This routine performs the classic binary tree deletion algorithm.
 * If the node to be deleted has a null link in either direction, we
 * just pull the non-null link up one to replace the existing link.
 * If both links exist, we instead delete the next link in order, which
 * is guaranteed to have a null link, then replace the node to be deleted
 * with the next link.
 */
void DeleteString (int p, LZSS_COMP_CTXT* ctxt)
{
    int  replacement;

    if ( ctxt->tree[ p ].parent == UNUSED )
        return;
    if ( ctxt->tree[ p ].larger_child == UNUSED )
        ContractNode( p, ctxt->tree[ p ].smaller_child, ctxt );
    else if ( ctxt->tree[ p ].smaller_child == UNUSED )
        ContractNode( p, ctxt->tree[ p ].larger_child, ctxt );
    else {
        replacement = FindNextNode( p, ctxt );
        DeleteString( replacement, ctxt );
        ReplaceNode( p, replacement, ctxt );
    }
}

/*
 * This where most of the work done by the encoder takes place.  This
 * routine is responsible for adding the new node to the binary tree.
 * It also has to find the best match among all the existing nodes in
 * the tree, and return that to the calling routine.  To make matters
 * even more complicated, if the new_node has a duplicate in the tree,
 * the old_node is deleted, for reasons of efficiency.
 */

int AddString (int new_node, int* match_position, LZSS_COMP_CTXT* ctxt)
{
    int i=0;
    int test_node=0;
    int delta=0;
    int match_length=0;
    int *child=NULL;

    if ( new_node == END_OF_STREAM )
        return( 0 );
    test_node = ctxt->tree[ TREE_ROOT ].larger_child;
    match_length = 0;
    for ( ; ; ) {
        for ( i = 0 ; i < LOOK_AHEAD_SIZE ; i++ ) {
            delta = ctxt->window[ MOD_WINDOW( new_node + i ) ] -
                    ctxt->window[ MOD_WINDOW( test_node + i ) ];
            if ( delta != 0 )
                break;
        }
        if ( i >= match_length ) {
            match_length = i;
            *match_position = test_node;
            if ( match_length >= LOOK_AHEAD_SIZE ) {
                ReplaceNode( test_node, new_node, ctxt );
                return( match_length );
            }
        }
        if ( delta >= 0 )
            child = &ctxt->tree[ test_node ].larger_child;
        else
            child = &ctxt->tree[ test_node ].smaller_child;
        if ( *child == UNUSED ) {
            *child = new_node;
            ctxt->tree[ new_node ].parent = test_node;
            ctxt->tree[ new_node ].larger_child = UNUSED;
            ctxt->tree[ new_node ].smaller_child = UNUSED;
            return( match_length );
        }
        test_node = *child;
    }
}

/*
 * This section of code and data makes up the blocked I/O portion of
 * the program.  Every token output consists of a single flag bit, followed
 * by either a single character or a index/length pair.  The flag bits
 * are stored in the first byte of a buffer array, and the characters
 * and index/length pairs are stored sequentially in the remaining
 * positions in the array.  After every eight output operations, the
 * first character of the array is full of flag bits, so the remaining
 * bytes stored in the array can be output.  This can be done with a
 * single fwrite() operation, making for greater efficiency.
 *
 * All that is needed to implement this is a few routines, plus three
 * data objects, which follow below.  The buffer has the flag bits
 * packed into its first character, with the remainder consisting of
 * the characters and index/length pairs, appearing in the order they
 * were output.  The FlagBitMask  is used to indicate where the next
 * flag bit will go when packed into DataBuffer[ 0 ].  Finally, the
 * BufferOffset is used to indicate where the next token will be stored
 * in the buffer.
 */

/*
 * To initialize the output buffer, we set the FlagBitMask to the first
 * bit position, can clear DataBuffer[0], which will hold all the
 * Flag bits.  Finally, the BufferOffset is set to 1, which is where the
 * first character or index/length pair will go.
 */

void InitOutputBuffer(LZSS_COMP_CTXT* ctxt)
{
    ctxt->DataBuffer[ 0 ] = 0;
    ctxt->FlagBitMask = 1;
    ctxt->OldBufferOffset = ctxt->BufferOffset ;
    ctxt->BufferOffset = 1;
}

/*
 * This routine is called during one of two different situations.  First,
 * it can potentially be called right after a character or a length/index
 * pair is added to the DataBuffer[].  If the position of the bit in the
 * FlagBitMask indicates that it is full, the output routine calls this
 * routine to flush data into the output file, and reset the output
 * variables to their initial state.  The other time this routine is
 * called is when the compression routine is ready to exit.  If there is
 * any data in the buffer at that time, it needs to be flushed.
 *
 * Note that this routine checks carefully to be sure that it doesn't
 * ever write out more data than was in the original uncompressed file.
 * It returns a 0 if this happens, which filters back to the compression
 * program, so that it can abort if this happens.
 *
 */

int FlushOutputBuffer(uchar *output_string, LZSS_COMP_CTXT* ctxt)
{
    if ( ctxt->BufferOffset == 1 )
        return( 1 );
    memcpy( output_string, ctxt->DataBuffer, ctxt->BufferOffset ) ;         /**/
    ctxt->compressed_size += ctxt->BufferOffset;                            /**/
    InitOutputBuffer(ctxt);
    return( 1 );
}

/*
 * This routine adds a single character to the output buffer.  In this
 * case, the flag bit is set, indicating that the next character is an
 * uncompressed byte.  After setting the flag and storing the byte,
 * the flag bit is shifted over, and checked.  If it turns out that all
 * eight bits in the flag bit character are used up, then we have to
 * flush the buffer and reinitialize the data.  Note that if the
 * FlushOutputBuffer() routine detects that the output has grown larger
 * than the input, it returns a 0 back to the calling routine.
 */

int OutputChar (int data, uchar *output_string, LZSS_COMP_CTXT* ctxt)
{
    ctxt->DataBuffer[ ctxt->BufferOffset++ ] = (uchar) data;
    ctxt->DataBuffer[ 0 ] |= ctxt->FlagBitMask;
    ctxt->FlagBitMask <<= 1;
    ctxt->inc_output_string=0;                                /**/
    if ( ctxt->FlagBitMask == 0x100 ){
        ctxt->inc_output_string=1;                            /**/
        return( FlushOutputBuffer(output_string,ctxt) );
    } else
        return( 1 );
}

/*
 * This routine is called to output a 12 bit position pointer and a 4 bit
 * length.  The 4 bit length is shifted to the top four bits of the first
 * of two DataBuffer[] characters.  The lower four bits contain the upper
 * four bits of the 12 bit position index.  The next of the two DataBuffer
 * characters gets the lower eight bits of the position index.  After
 * all that work to store those 16 bits, the FlagBitMask is shifted over,
 * and checked to see if we have used up all our bits.  If we have,
 * the output buffer is flushed, and the output data elements are reset.
 * If the FlushOutputBuffer routine detects that the output file has
 * grown too large, it passes and error return back via this routine,
 * so that it can abort.
 */

int OutputPair( int position, int length, uchar *output_string, LZSS_COMP_CTXT* ctxt)
{
    ctxt->DataBuffer[ ctxt->BufferOffset ] = (uchar) ( length << 4 );
    ctxt->DataBuffer[ ctxt->BufferOffset++ ] |= ( position >> 8 );
    ctxt->DataBuffer[ ctxt->BufferOffset++ ] = (uchar) ( position & 0xff );
    ctxt->FlagBitMask <<= 1;
    ctxt->inc_output_string=0;                                /**/
    if ( ctxt->FlagBitMask == 0x100 ){
        ctxt->inc_output_string=1;                            /**/
        return( FlushOutputBuffer(output_string,ctxt) );
    } else
        return( 1 );
}

/*
 * The input process uses the same data structures as the blocked output
 * routines, but it is somewhat simpler, in that it doesn't actually have
 * to read in a whole block of data at once.  Instead, it just reads in
 * a single character full of flag bits into DataBuffer[0], and passes
 * individual bits back to the Expansion program when asked for them.
 * The expansion program is left to its own devices for reading in the
 * characters, indices, and match lengths.  They can be read in sequentially
 * using normal file I/O.
 */

void InitInputBuffer (uchar *input_string, LZSS_COMP_CTXT* ctxt)				/**/
{
    ctxt->FlagBitMask = 1;
    ctxt->DataBuffer[ 0 ] = *input_string;										/**/
}

/*
 * When the Expansion program wants a flag bit, it calls this routine.
 * This routine has to keep track of whether or not it has run out of
 * flag bits.  If it has, it has to go back and reinitialize so as to
 * have a fresh set.
 */

int InputBit (uchar *input_string, LZSS_COMP_CTXT* ctxt)						/**/
{
    ctxt->inc_input_string=0;
    if ( ctxt->FlagBitMask == 0x100 ) {
        InitInputBuffer(input_string,ctxt);										/**/
        ctxt->inc_input_string=1;
        }
    ctxt->FlagBitMask <<= 1;
    return( ctxt->DataBuffer[ 0 ] & ( ctxt->FlagBitMask >> 1 ) );
}

/*
 * This is the compression routine.  It has to first load up the look
 * ahead buffer, then go into the main compression loop.  The main loop
 * decides whether to output a single character or an index/length
 * token that defines a phrase.  Once the character or phrase has been
 * sent out, another loop has to run.  The second loop reads in new
 * characters, deletes the strings that are overwritten by the new
 * character, then adds the strings that are created by the new
 * character.  While running it has the additional responsibility of
 * creating the checksum of the input data, and checking for when the
 * output data grows too large.  The program returns a success or failure
 * indicator.  
 *
 */
/*-----------------------------------------------------------------------
        The changes I have made to this function are marked by an empty
   comment.  All of the CRC data stuff has been deleted.
        The output_string now gets sent to all the functions that
   call FlushOuputBuffer, and in FlushOutputBuffer, the buffer is catted
   to the output_string.  Also, where before a call to getc was used to
   fill the window array, I have substituted 
                c= *input;
                input++;
   In addition, the size parameter is decremented everytime the window
   array is filled.  Once  size<=0, the exit condition is initiated
   (look_ahead_bites => 0).
   
   Dave Lewak, 7/25/96
-----------------------------------------------------------------------*/

#ifdef C_LINKAGE
extern "C"
{
#endif

int LZSS_Compress (uchar *input_string, uchar *output_string, int size)
{
    int i;
    uchar c;
    int look_ahead_bytes;
    int current_position;
    int replace_count;
    int match_length;
    int match_position;
	LZSS_COMP_CTXT ctxt;
	
	// OW
	memset(&ctxt, 0, sizeof(ctxt));

    ctxt.compressed_size = 0;                                /**/
    ctxt.original_size = size;                               /**/
    InitOutputBuffer(&ctxt);

    current_position = 1;
    for ( i = 0 ; i < LOOK_AHEAD_SIZE ; i++ ) {
        c=*input_string;                                /**/
        input_string++;                                 /**/
        size--;                                         /**/
        if ( size<0)                                    /**/
            break;
        ctxt.window[ current_position + i ] = c;
    }
    look_ahead_bytes = i;
    InitTree( current_position, &ctxt );
    match_length = 0;
    match_position = 0;
    while ( look_ahead_bytes > 0 ) {
        if ( match_length > look_ahead_bytes ){
            match_length = look_ahead_bytes;
		}
        if ( match_length <= BREAK_EVEN ) {
            replace_count = 1;
            if ( !OutputChar( ctxt.window[ current_position ], output_string, &ctxt ) ){
                return( 0 );
			}
            if(ctxt.inc_output_string){
                output_string+=ctxt.OldBufferOffset;
			}
        } else {
            if ( !OutputPair( match_position, 
                    match_length - ( BREAK_EVEN + 1 ), output_string, &ctxt ) ){
                return( 0 );
			}
            if(ctxt.inc_output_string){
                output_string+=ctxt.OldBufferOffset;
			}
            replace_count = match_length;
        }
        for ( i = 0 ; i < replace_count ; i++ ) {
            DeleteString( MOD_WINDOW( current_position + LOOK_AHEAD_SIZE ), &ctxt );
            c=*input_string;  		                    /**/
            size--;                                     /**/
            if ( size<0) {                              /**/
                look_ahead_bytes--;
            } else {
				//Only increment while the end of the input string 
				//hasn't been reached
	            input_string++;                             /**/

                ctxt.window[ MOD_WINDOW( current_position + LOOK_AHEAD_SIZE ) ] = c;
            }
            current_position = MOD_WINDOW( current_position + 1 );
            if ( look_ahead_bytes )
                match_length = AddString( current_position, &match_position, &ctxt );
        }
    }


    /* If the previous OutputChar or OutputPair call
       didn't write to the output, do so now */
    if(!ctxt.inc_output_string)
        FlushOutputBuffer(output_string, &ctxt);

    return( ctxt.compressed_size );
}

/*
 * This is the expansion routine for the LZSS algorithm.  All it has
 * to do is read in flag bits, decide whether to read in a character or
 * a index/length pair, and take the appropriate action.  It is responsible
 * for keeping track of the crc of the output data, and must return it
 * to the calling routine, for verification.
 */

//sfr: added the src size here, we cant read past it
int LZSS_Expand (uchar *input_string, int srcSize, uchar *output_string, int size) {
	int i;
	int current_position;
	uchar c;
	int match_length;
	int match_position;
	unsigned long input_count;
	uchar *inputHead;
	LZSS_COMP_CTXT ctxt;

	if (size <= 0){
		return -1;
	}
	
	inputHead=input_string;

	
	InitInputBuffer(input_string, &ctxt);               /**/
	CHSZ(srcSize, 1);	
	input_string++;                                     /**/
	current_position = 1;

	// While we still have room in the output buffer
	//sfr: added check for source also
	while ( size  /*&& srcSize*/) {
		CHSZ(srcSize, 1);
		if ( InputBit(input_string, &ctxt) ) {
			// We're going to write a single characters

			/* InputBit if calls InitInputBuffer, 
			   then increment input_string */
			if(ctxt.inc_input_string==1){                /**/
				CHSZ(srcSize, 1);
				input_string++;                         /**/
			}
			c = *input_string;                          /**/

			/* Exit Condition */
			//    if(c==0)                                  /**/
			//      break;                                  /**/

			CHSZ(srcSize, 1);
			input_string++;                             /**/
			*output_string=c;                           /**/
			output_string++;                            /**/
			size--;
			ctxt.window[ current_position ] = c;
			current_position = MOD_WINDOW( current_position + 1 );
		} else {
			// We're going to write a match from the code book

			if(ctxt.inc_input_string==1){                /**/
				CHSZ(srcSize, 1);
				input_string++;                         /**/
			}
			match_length = *input_string;               /**/
			CHSZ(srcSize, 1);
			input_string++;                             /**/
			match_position = *input_string;             /**/
			CHSZ(srcSize, 1);
			input_string++;                             /**/
			match_position |= ( match_length & 0xf ) << 8;
			match_length >>= 4;
			match_length += BREAK_EVEN;

			// This if prevents us from overrunning the output buffer if the last
			// match we find is longer than the remaining space in the output buffer.
			// This might best be fixed in the Compression routine, but it was much
			// easier to do here, so...  SCR 11/11/98
			if (match_length < size) {
				// Normal case
				size -= match_length + 1;
			} else {
				// End case
				size = 0;
				match_length = size-1;
			}

			// Write the code word into the output buffer
			for ( i = 0 ; i <= match_length ; i++ ) {
				c = ctxt.window[ MOD_WINDOW( match_position + i ) ];
				*output_string=c;
				output_string++;
				ctxt.window[ current_position ] = c;
				current_position = MOD_WINDOW( current_position + 1 );
			}
		}
	}
	input_count=input_string-inputHead;
	return( input_count);
}

#ifdef INCLUDE_FILE_COMPRESSION

/*
 * This is the compression routine.  It has to first load up the look
 * ahead buffer, then go into the main compression loop.  The main loop
 * decides whether to output a single character or an index/length
 * token that defines a phrase.  Once the character or phrase has been
 * sent out, another loop has to run.  The second loop reads in new
 * characters, deletes the strings that are overwritten by the new
 * character, then adds the strings that are created by the new
 * character.
 */

unsigned long LZSS_CompressFile( FILE *input, BIT_FILE *output )
	{
    int i;
    int c;
    int look_ahead_bytes;
    int current_position;
    int replace_count;
    int match_length;
    int match_position;
	LZSS_COMP_CTXT ctxt;

	/* getting file compression size */
	unsigned long	curPos, finalPos;

	curPos = ftell( output->file );

    current_position = 1;
    for ( i = 0 ; i < LOOK_AHEAD_SIZE ; i++ ) {
        if ( ( c = getc( input ) ) == EOF )
            break;
        ctxt.window[ current_position + i ] = (unsigned char) c;
    }
    look_ahead_bytes = i;
    InitTree( current_position, &ctxt );
    match_length = 0;
    match_position = 0;
    while ( look_ahead_bytes > 0 ) {
        if ( match_length > look_ahead_bytes )
            match_length = look_ahead_bytes;
        if ( match_length <= BREAK_EVEN ) {
            replace_count = 1;
            OutputBit( output, 1 );
            OutputBits( output, (unsigned long) ctxt.window[ current_position ], 8 );
        } else {
            OutputBit( output, 0 );
            OutputBits( output, (unsigned long) match_position, INDEX_BIT_COUNT );
            OutputBits( output, (unsigned long) ( match_length - ( BREAK_EVEN + 1 ) ), LENGTH_BIT_COUNT );
            replace_count = match_length;
        }
        for ( i = 0 ; i < replace_count ; i++ ) {
            DeleteString( MOD_WINDOW( current_position + LOOK_AHEAD_SIZE ), &ctxt );
            if ( ( c = getc( input ) ) == EOF )
                look_ahead_bytes--;
            else
                ctxt.window[ MOD_WINDOW( current_position + LOOK_AHEAD_SIZE ) ] = (unsigned char) c;
            current_position = MOD_WINDOW( current_position + 1 );
            if ( look_ahead_bytes )
                match_length = AddString( current_position, &match_position, &ctxt );
        }
    };
    OutputBit( output, 0 );
    OutputBits( output, (unsigned long) END_OF_STREAM, INDEX_BIT_COUNT );

	
	finalPos = ftell( output->file );

	curPos = finalPos-curPos;

	return( curPos );
	}

/*
 * This is the expansion routine for the LZSS algorithm.  All it has
 * to do is read in flag bits, decide whether to read in a character or
 * a index/length pair, and take the appropriate action.
 */

void LZSS_ExpandFile( BIT_FILE *input, FILE *output )
{
	int i;
	int current_position;
	int c;
	int match_length;
	int match_position;
	LZSS_COMP_CTXT ctxt;

	current_position = 1;
	for ( ; ; ) 
	{
		if ( InputBit( input ) ) 
		{
			c = (int) InputBits( input, 8 );
			putc( c, output );
			ctxt.window[ current_position ] = (unsigned char) c;
			current_position = MOD_WINDOW( current_position + 1 );
		} 
		else 
		{
			match_position = (int) InputBits( input, INDEX_BIT_COUNT );
			if ( match_position == END_OF_STREAM )
				break;
			match_length = (int) InputBits( input, LENGTH_BIT_COUNT );
			match_length += BREAK_EVEN;
			for ( i = 0 ; i <= match_length ; i++ ) 
			{
				c = ctxt.window[ MOD_WINDOW( match_position + i ) ];
				putc( c, output );
				ctxt.window[ current_position ] = (unsigned char) c;
				current_position = MOD_WINDOW( current_position + 1 );
			}
		}
	}
}

/*
 * BUFFER EXPANSION: This is the expansion routine for the LZSS algorithm.  
 * All it has to do is read in flag bits, decide whether to read in a character 
 * or a index/length pair from a buffer, and take the appropriate action.
 */

unsigned long LZSS_ReadFile(unsigned long bytesToRead, BIT_FILE *input, unsigned char **buffer, unsigned char **fill_level)
	{
    int i;
    int c;
	int	inputRet;
    unsigned long byteCount;
    unsigned char *ptr;
	LZSS_COMP_CTXT ctxt;

	if (input->match_position == END_OF_STREAM) 
		{
        return 0;
		}

    ptr = *fill_level;//*buffer;
    byteCount = 0; 
    while (byteCount < bytesToRead) 
		{
		inputRet = InputBit(input);
		if( inputRet == EOF )
			break;
		

		// TEMP FIX
	/*	if ( ptr >= tempFileEnd )
			break;*/

        if (inputRet)
			{
            c = (int) InputBits(input, 8);
            *ptr = c;
            ptr++;
            byteCount++;
            ctxt.window[ input->current_position ] = (unsigned char) c;
            input->current_position = MOD_WINDOW(input->current_position + 1);
			} 
        else 
			{
            input->match_position = (int) InputBits(input, INDEX_BIT_COUNT);
            if (input->match_position == END_OF_STREAM) 
				{
                break;
				}
            input->match_length = (int)InputBits(input, LENGTH_BIT_COUNT);
            input->match_length += BREAK_EVEN;
            for (i = 0; i <= input->match_length; i++) 
				{
                c = ctxt.window[MOD_WINDOW(input->match_position + i)];
                *ptr = c;
                ptr++;
                byteCount++;
                ctxt.window[ input->current_position ] = (unsigned char) c;
                input->current_position = MOD_WINDOW(input->current_position + 1);
				}
			}
		}

    *fill_level = ptr;
    input->bytesRead = input->bytesRead + byteCount;
    return byteCount;
	}

#endif

#ifdef C_LINKAGE
} // extern "C"
#endif


/**************************     End of lzss.c     *************************/
