/*
 *      NEWCON.C
 *
 *      by David Schultz [CIS 73157,2242]
 *
 *      This file was created by examining the code in the conbdos.o
 *      module in the CPMLIB library from CP/M-68K v.1.2. The
 *      readline() function was then rewritten and enhanced.
 *
 */

#ifdef INIT
extern char     **def_dma;      /* default disk dma address from bios   */
#endif

extern struct {                 /* offset */
        char    count;          /* 0x00   */
        char    delim;          /* 0x01   */    /* string delimiter '$' */
        char    p;              /* 0x02   */    /* printer echo switch  */
        char    flag;           /* 0x03   */    /* controls DEL action  */
        unsigned int cur_pos;   /* 0x04   */    /* tracks cursor position */
        int     unk2[49];
        char    *bp1;           /* 0x68  */     /* put next char here   */
        char    *bp2;           /* 0x6c  */     /* get next char here   */
        char    buffer[80];     /* 0x70  */     /* type ahead buffer    */
        } gbls;

extern int      bios1();
extern int      bios2();

#define CTLA    0x01
#define CTLB    0x02
#define CTLC    0x03
#define CTLE    0x05
#define CTLF    0x06
#define CTLG    0x07
#define BS      0x08
#define TAB     0x09
#define LF      0x0a
#define CTLK    0x0b
#define CR      0x0d
#define CTLP    0x10
#define XON     0x11
#define CTLR    0x12
#define XOFF    0x13
#define CTLU    0x15
#define CTLW    0x17
#define CTLX    0x18
#define SPACE   ' '
#define DEL     0x7f

/*----------------------------------------------------------------------*/

/*
 *      Return the current status of the console device. If there are
 *      characters buffered, return 1. Otherwise querry the bios for
 *      the status.
 *
 *      Returns:        0 - not ready
 *                      1 - ready ( buffered character )
 *                   0xff - ready ( device )
 */
 
constat()
{
        if (gbls.count != 0)

                return(1);
        return bios1(2);
}

/*----------------------------------------------------------------------*/

/*
 *      This function handles that marvalous feature - typeahead.
 *      It also checks for XON/XOFF, ^C, and the printer switch
 */

conbrk()
{
        register char   ch;
        register char   stop = 0;

        if (bios1(2) != 0)      /* see if char is available     */
        {
                do              /* yes, check for special action        */
                {               /* or buffer it.                        */
                
                        if ((ch = bios1(3)) == CTLC)    /* if char is ^C */
                                warmboot(2);            /* warmboot     */

                        /* Check for the XOFF character. This stops
                         * console output. If XOFF is recieved, stay
                         * in this loop until a XON is recieved.
                         */

                        if (ch == XOFF)
                        {
                                stop = 1;
                                continue;
                        }
                        if (ch == XON)
                        {
                                stop = 0;
                                continue;
                        }

                        /* Check for the printer echo toggle char
                         */
                         
                        if (ch == CTLP)
                        {
                                gbls.p = ( gbls.p ? 0 : 1 );
                                continue;
                        }

                        /* If not a special char, then place into buffer
                         * unless buffer is full.
                         */

                        if (gbls.count < 126)
                        {
                                *gbls.bp1++ = ch;
                                gbls.count++;
                        }
                }
                while (stop != 0);
        }
}

/*----------------------------------------------------------------------*/

/*
 *      Send a char to the console device.
 */

conout(ch)
register char   ch;
{
        /* First check for XON/XOFF, ^C, CTLP, or input to buffer.
         */
         
        conbrk();

        bios2(4, ch);           /* send char to console.        */

        if (gbls.p != 0)        /* If printer echo is on, send it       */
                bios2(5, ch);   /* the printer also.                    */

        /* Now update the cursor position counter
         */
         
        if (ch >= ' ')                  /* replace blt with bcs */
                gbls.cur_pos++;
        else if(ch == CR)
                gbls.cur_pos = 0;
        else if(ch == BS)
                gbls.cur_pos--;
}

/*----------------------------------------------------------------------*/

/*
 *      Send a char to the console with tab expansion. Tabs are at
 *      every 8 columns.
 */

tabout(ch)
register char   ch;
{
        if(ch == TAB)
                do
                        conout(SPACE);
                while (gbls.cur_pos & 0x07);
        else
                conout(ch);
}

/*----------------------------------------------------------------------*/

/*
 *      Send a char to the console with tab expansion and expand
 *      non-printing control characters to the sequence
 *      '^' '(ch | '2')'. ie. 0x03 to ^C.
 */

cookdouble(ch)
register char   ch;
{
        if( ch == TAB )
                tabout(ch);
        
        else
        {
                if( ch < ' ' )  /* replace bge with bcc */
                {
                        conout('^');
                        ch |= '@';
                }
                conout(ch);
        }
}

/*----------------------------------------------------------------------*/

/*
 *      Get a character from the console. Check for buffered input first.
 */

getch()
{
        register char   temp;
        
        if( gbls.count != 0 )
        {
                temp = *gbls.bp2;
                gbls.bp2++;
                gbls.count--;
                
                /* Note that this is the only place that the buffer
                 * pointers bp1 and bp2 are reset to the start of
                 * the buffer.
                 */
                 
                if( gbls.count == 0 )
                        gbls.bp2 = gbls.bp1 = gbls.buffer;

                return temp;
        }
        return bios1(3);
}

/*----------------------------------------------------------------------*/

/*
 *      Get a character and echo it to the console. Also check to
 *      see if it is the printer echo toggle.
 */

conin()
{
        register char   ch;
        
        conout( ch = getch() );
        
        if( ch == CTLP )
                gbls.p = gbls.p ? 0 : 1;
                
        return ch;
}

/*----------------------------------------------------------------------*/

/*
 *      Raw console i/o. This function handles input, output, and status
 *      Since conbrk() is not called explicitly or implicitly, no
 *      action is taken on any of the characters(XON/XOFF etc.)
 */

rawconin(parm)
register int    parm;
{
        if( parm == 0xff )
                return getch();
        else if( parm == 0xfe )
                return constat();
        else
                bios2( 4, (parm & 0xff) );
}

/*----------------------------------------------------------------------*/

/*
 *      Send the string with tab expansion. Output stops when a char
 *      matches gbls.delim.
 */

prt_line( p )
register char   *p;
{
        while( *p != gbls.delim )
                tabout( *p++ );
}

/*======================================================================*/
/*      The following functions are used by readline().                 */

/*----------------------------------------------------------------------*/

/*
 *      Insert a character at the current position in the buffer, and
 *      update the display.
 */
 
insert( bufp, j, ch )
register char   *bufp;
register int    j;
char    ch;
{
        register int    i;
        register char           *p;
        int             stcol;
        
        i = bufp[1] & 0xff;
        
        /* Move buffer over by one char
         */

        p = bufp + i + 2;
        
        do {
                *(p+1) = *p;
                p--;
                i--;
        } while( i >= j );
        
        /* Insert character
         */

        bufp[j+2] = ch;
        bufp[1]++;
        j++;

        /* display new character and remember where we are.
         */

        cookdouble( ch );
        stcol = gbls.cur_pos;
        
        /* display to end of line.
         */

        while( j < (bufp[1] & 0xff) )
        {
                cookdouble( bufp[j+2] );
                j++;
        }

        /* Back up to where we were ( after the inserted character )
         */

        while( stcol < gbls.cur_pos )
                conout( BS );
}
        
/*
 *      Delete the character and update display
 */

delch( bufp, j )
register char   *bufp;
register int    j;
{
        register int    i;
        register char   *p;
        int             stcol;
        
        
        /* If at end of line, do nothing
         */

        if( j < (bufp[1] & 0xff) )
        {
                /* Move buffer over by one position, deleteing char
                 * at current position.
                 */

                i = j;
                p = bufp + j + 2;

                while( i < (bufp[1] & 0xff) )
                {
                        *p = *(p+1);
                        p++;
                        i++;
                }
        
                /* update count of chars in string
                 */

                bufp[1]--;

                /* Now remember where we are and redisplay from current
                 * position to end of line. Note that if tabs are in
                 * string or was deleted, some trash will be left at
                 * the end of the line.
                 */

                stcol = gbls.cur_pos;
        
                while( j < (bufp[1] & 0xff) )
                {
                        cookdouble( bufp[j+2] );
                        j++;
                }
                conout( SPACE );
                conout( SPACE );        /* just to be sure      */

                /* Now return cursor to where we were before.
                 */

                while( stcol < gbls.cur_pos )
                        conout( BS );
        }
}

/*
 *      Delete to end of line.
 */

deol( bufp, j )
register char   *bufp;
int     j;
{
        register int    i;
        int     stcol;
        
        /* If already at the end of line, there is nothing to do.
         */

        if( j < (bufp[1] & 0xff) )
        {
                /* remember where we are */

                stcol = gbls.cur_pos;
                i = j;          
                
                /* Now erase all the characters on the display to end
                 * of line.
                 */

                while( i < (bufp[1] & 0xff) )
                {
                        if( bufp[i+2] == TAB )
                                cookdouble( TAB );
                        else if( (bufp[i+2] & 0xff) < SPACE )
                        {
                                conout(SPACE);
                                conout(SPACE);
                        }
                        else
                                conout(SPACE);
                        i++;
                }
                
                /* Move back to where we started.       */

                while( stcol < gbls.cur_pos )
                        conout(BS);

                /* Now update the length counter        */

                bufp[1]   = j;
        }
}

/*
 *      Delete to beginning of line.
 */

dbol( bufp, stcol, j )
register char   *bufp;
int             stcol, j;
{
        register int    i, k;
        
        /* First, move to the end of the line
         */

        i = j;
        while( i < (bufp[1] & 0xff) )
        {
                cookdouble( bufp[i+2] );
                i++;
        }
        
        /* Now backup and erase the entire line */
        
        while( stcol < gbls.cur_pos )
        {
                conout( BS );
                conout( SPACE );
                conout( BS );
        }
        
        /* Now move the buffer over and update the count of characters  */

        i = 0;
        k = j;
        
        while( k < (bufp[1] & 0xff) )
        {
                bufp[i+2] = bufp[k+2];
                i++;
                k++;
        }
        bufp[1] = (bufp[1] & 0xff) - j;
        
        /* Redisply whats left  */

        i = 0;
        
        while( i < (bufp[1] & 0xff))
        {
                cookdouble( bufp[i+2] );
                i++;
        }
        
        /* And backup to beginning of line      */

        while( stcol < gbls.cur_pos )
                conout( BS );

        /* That's a lot of work, isn't it?      */
}
        
/*
 *      Move to the next line and move over to startcol.
 *      Used only by readline().
 */

newline( startcol )
register int    startcol;
{
        conout( CR );
        conout( LF );
        
        while( startcol  )
        {
                conout( ' ' );
                startcol--;
        }
}

/*----------------------------------------------------------------------*/

/*
 *      Backup the display by one character taking into account
 *      tabs and control characters.
 */

backsp( bufp, col, i )
register char   *bufp;
register unsigned int   col;
register int            i;
{
        register char   ch;
        register char   *p;
        
        p = &bufp[2];
        
        /* First, find out where we want to go to. On entry col contains
         * the column of the leftmost character. We count displayed
         * characters from that point up to one character less than
         * where we are now.
         */

        while( i-- )
        {
                ch = *p++;
                if( ch == TAB )
                        col = (col + 8) & 0xfff8;

                /*
                 * This comparison could cause some trouble if you
                 * try to display a non-ASCII character. ie. one
                 * with 8 bits. If you need to display these chars
                 * change <ch> to <(ch & 0xff)>
                 */

                else if( ch < ' ' )
                        col += 2;
                else
                        col++;
        }
        /*
         *      And then backup to that position.
         */

        while( col < gbls.cur_pos )
        {
                conout( BS );
        }
}
 
/*----------------------------------------------------------------------*/

/*
 *      Read a line of input with editing.
 */

readline( p )
register char   *p;
{
        register char   ch;
        register unsigned       i;
        register        j;              /* current position is string   */
        register char   *q;
        int             stcol;

        /*      Remember where we are on current line.
         */
         
        stcol = gbls.cur_pos;

        /*      Initialize position to beginning of string.
         */
         
        j = 0;
        
        /*      If we are passed a null pointer, then an initialized
         *      string is present at the default dma address.
         *      The format of an initialized string is:
         *
         *      mx  nc  c1  c2  c3 ... 0
         *
         *      mx is the maximum number of characters the string can hold.
         *      nc is the number of characters in the string. This does
         *      not need to be inititialized before calling this function.
         *      It will be valid on exit.
         *
         *      c1 c2 ... is a null terminated string. This string will
         *      be printed and available for editing.
         *
         *      This has not been tested because my bios uses the symbol
         *      dma for its dma pointer. I'll have to change it to
         *      _dma to test this.
         */

#ifdef INIT

        if( p == 0 )
        {
                p = def_dma;    /* get dma address from bios    */
                
                /* Print the string and update count and position
                 */
                 
                while( p[j+2] && (( p[1] & 0xff) < (*p &0xff) ) )
                {
                        conout(p[j+2]);
                        p[1]++;
                        j++;
                }
        }
        else
#endif
                p[1] = 0;
                
        while(1)
        {
                switch( ch = getch() )
                {
                case CR:
                case LF:
                        /* Return line after saving it for later recall
                         */
                        saveline( p );
                        conout(CR);
                        return;

                case CTLA:
                        /* Backup one char      */
                        
                        if( j > 0 )
                        {
                                j--;
                                backsp( p, stcol, j );
                        }
                        break;

                case CTLB:
                        /* move to beginning (or end) of line.  */

                        if( j > 0 )
                        {
                                while( j > 0 )
                                {
                                        j--;
                                        backsp( p, stcol, j );
                                }
                        }
                        else
                        {
                                while( j < (p[1] & 0xff) )
                                {
                                        cookdouble(p[j+2]);
                                        j++;
                                }
                        }
                        break;

                case CTLE:
                        /* move to new line and continue input  */

                        newline( stcol );
                        break;

                case CTLF:
                        /* move left one character      */

                        if( j < (p[1] & 0xff) )
                        {
                                cookdouble(p[j+2]);
                                j++;
                        }
                        break;

                case CTLG:
                case DEL:
                        /* If at end of line, delete previous character.
                         * By falling through to BS code.
                         */
                        
                        if( j < (p[1] & 0xff) )
                        {
                                /* delete character     */
                                delch( p, j );
                                break;
                        }

                case BS:
                        /* delete previous character    */

                        if( j > 0 )
                        {
                                j--;
                                backsp( p, stcol, j );
                                delch( p, j );
                        }
                        break;

                case CTLK:
                        /* delete to end of line        */

                        deol( p, j );
                        break;

                case CTLR:
                        /* retype line                  */

                        conout('#');
                        newline( stcol );
                        j = 0;
                        
                        while( j < (p[1] & 0xff) )
                        {
                                cookdouble(p[j+2]);
                                j++;
                        }
                        break;

                case CTLU:
                        /* save line contents and then discard  */

                        saveline( p );
                        
                        conout('#');
                        newline( stcol );
                        p[1] = 0;
                        j = 0;
                        break;

                case CTLW:
                        /* recall previously saved line */

                        if( p[1] == 0 )
                                getline( p );
                        j = 0;
                        while( j < (p[1] & 0xff) )
                        {
                                cookdouble( p[j+2] );
                                j++;
                        }
                        break;

                case CTLX:
                        /* delete to beginning of line  */

                        dbol( p, stcol, j );
                        j = 0;
                        break;

                case CTLC:
                        /* do a warmboot
                         * NOTE: this must be the last case before
                         * the default.
                         */

                        if( p[1] == 0 )
                        {
                                cookdouble( CTLC );
                                warmboot(2);
                        }
                        /* fall through */

                default:
                        if( (p[1] & 0xff) < (*p & 0xff) )
                        {
                                insert(p, j, ch);
                                j++;
                        }
                }
        }
}

/*----------------------------------------------------------------------*/

static char savebuf[257] = { 0, 0 };

/*
 *      Save a line for future recall.
 */

saveline( bufp )
register char   *bufp;
{
        register char   *p, *q;
        register int    i;
        
        /* Just copy the input buffer into the save buffer      */

        p = savebuf;
        q = bufp;
        
        for( i = 0; i <= ((bufp[1] & 0xff) + 2) ; i++ )
                *p++ = *q++;
}

/*
 *      Recall a previously saved line. Makeing sure not to overrun
 *      the current buffer.
 */

getline( bufp )
register char   *bufp;
{
        register char   *p, *q;
        register int    i, count;
        
        p = savebuf;
        q = bufp;
        count = (p[1] & 0xff) > (*bufp & 0xff) ? *bufp : p[1] ;
        
        for( i = 0; i < count ; i++ )
        {
                *(q+2) = *(p+2);
                q++;
                p++;
        }

        bufp[1] = i;
}

