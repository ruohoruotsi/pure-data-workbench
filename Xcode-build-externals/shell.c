//-----------------------------------------------------------------------------
// shell.c - Pd example object
//
// Accepts mixed float / symbol input and sorts it to corresponding outlets
//
// Cooper Baker 2010
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// m_pd.h - main header for Pd
//-----------------------------------------------------------------------------
#include "m_pd.h"
#include "stdio.h"
#include "string.h"

#define STRING_MAX 65535

//-----------------------------------------------------------------------------
// shell_class - pointer to this object's definition 
//-----------------------------------------------------------------------------
t_class* shell_class;


//-----------------------------------------------------------------------------
// shell - data structure holding this object's data
//-----------------------------------------------------------------------------
typedef struct shell
{
    // this object - must always be first variable in struct
    t_object object;
  
    // pointer to the outlet
    t_outlet* outlet;

    // character string to hold a shell command
    char* shell_command[ 1024 ];

} t_shell;


//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
char* shell_itoa  ( int number );
void  shell_parse ( t_shell* object, t_symbol* selector, int items, t_atom* list );
void  shell_bang  ( t_shell* object );
void* shell_new   ( void );
void  shell_setup ( void );


//-----------------------------------------------------------------------------
// shell_itoa - converts integers to ascii strings
//-----------------------------------------------------------------------------
char* shell_itoa( int number )
{
    int i = 0;
    int j = 0;
    int sign = number;
    char character;
    static char string[ STRING_MAX ];
    
    // if number is negative
    if( sign < 0 )
    {
        // make the number positive
        number = -number;
    }
    
    // generate digits in reverse order
    for( number ; number > 0 ; number /= 10 )
    {
        // calculate and store ascii values
        string[ i ] = number % 10 + '0';
        ++i;
    }
    
    // if number is negative
    if( sign < 0 )
    {
        // add the minus sign
        string[ i++ ] = '-';
    }

    // null terminate the string
    string[ i ] = '\0';

    // reverse the string
    for( i = 0, j = strlen( string ) - 1 ; i < j ; ++i, --j )
    {
        character   = string[ i ];
        string[ i ] = string[ j ];
        string[ j ] = character;
    }

    // hand the string to the calling function
    return string;
}


//-----------------------------------------------------------------------------
// shell_bang - causes status list output
//-----------------------------------------------------------------------------
void shell_bang( t_shell* object )
{
    // an os FILE pointer for the pipe
    FILE* shell_pipe;

    // string to hold what comes out of the pipe
    char output[ STRING_MAX ];

    // open a "read" pipe and send it the object's command
    shell_pipe = popen( ( char* )object->shell_command, "r" );
    
    // move through the output to its end
    while( fgets( output, STRING_MAX, shell_pipe ) != NULL )
    {
        // post the output in the status window
        post( output );
        
        // send the output out the object's outlet
        outlet_symbol( object->outlet, gensym( output ) );
    }

    // close the pipe
    pclose( shell_pipe );
}


//-----------------------------------------------------------------------------
// shell_parse - parses list input
//-----------------------------------------------------------------------------
void shell_parse( t_shell* object, t_symbol* selector, int items, t_atom* list )
{
    // index to keep track of position in input list
    int item_index = 0;
    
    // clear out the last command
    memset( object->shell_command, 0, 1024 * sizeof( char ) );
    
    // copy the first word into the command string
    strcpy( ( char* )object->shell_command, selector->s_name );
    
    // iterate through the input mesage list
    for( item_index = 0 ; item_index < items ; ++item_index )
    {
        // if the list item is a symbol
        if( list[ item_index ].a_type == A_SYMBOL )
        {
            // first add a space then copy the item into the command string
            strcat( ( char* )object->shell_command, " " );
            strcat( ( char* )object->shell_command, list[ item_index ].a_w.w_symbol->s_name );
        }
        // if the list item is a float
        else if( list[ item_index ].a_type == A_FLOAT )
        {
            // first add a space then copy the item into the command string
            strcat( ( char* )object->shell_command, " " );
            strcat( ( char* )object->shell_command, shell_itoa( list[ item_index ].a_w.w_float ) );
        }        
    }

    post( (const char*)object->shell_command );

    // call bang to trigger output
    shell_bang( object );
}


//-----------------------------------------------------------------------------
// shell_new - initialize the object upon instantiation ( aka "constructor" )
//-----------------------------------------------------------------------------
void* shell_new( void )
{
    // declare a pointer to this class 
    t_shell* object;
    
    // generate a new object and save its pointer in "object"
    object = ( t_shell* )pd_new( shell_class );

    object->outlet = outlet_new( &object->object, gensym( "anything" ) );
    
    // return the pointer to this class
    return ( void* )object;
}


//-----------------------------------------------------------------------------
// shell setup - defines this object and its properties to Pd
//-----------------------------------------------------------------------------
void shell_setup( void )
{
    // create a new class and assign its pointer to shell_class
    shell_class = class_new( gensym( "shell" ), ( t_newmethod )shell_new, 0, sizeof( t_shell ), 0, 0 );

    // add message handler, responding to any message
    class_addmethod( shell_class, ( t_method )shell_parse, gensym( "anything" ), A_GIMME, 0 );    
 
    // add bang handler
    class_addbang( shell_class, ( t_method )shell_bang );                  
}


//-----------------------------------------------------------------------------
// EOF
//-----------------------------------------------------------------------------
