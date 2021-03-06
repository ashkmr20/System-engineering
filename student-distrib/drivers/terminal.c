#include "terminal.h"



char keyscan []={' ','1','2','3','4','5','6','7','8','9','0','-','=',' ',' ','q','w','e','r','t','y','u','i','o','p','[',']',' ',' ','a',
's','d','f','g','h','j','k','l',';','\'','`',' ','\\','z','x','c','v','b','n','m',',','.','/',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

char shift_keyscan []={' ','!','@','#','$','%','^','&','*','(',')','_','+',' ',' ','Q','W','E','R','T','Y','U','I','O','P','{','}',' ',' ','A',
'S','D','F','G','H','J','K','L',':','"','~',' ','|','Z','X','C','V','B','N','M','<','>','?',
' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

/* init_terminal
 *    INPUT: none
 * FUNCTION: sets initial global vars for terminal (clears buffer etc)
 *           clear screen
 */
void init_terminal(void)
{
    shift = 0;
    capslock = 0;
    ctrl_l = 0;
    alt_l = 0;
    term_loc[0] = 0;
    term_loc[1] = 0;
    term_loc[2] = 0;
    entered[0] = 0;
    entered[1] = 0;
    entered[2] = 0;
    current_terminal = 0;
    buff_to_top();
}

/* process_key
 *    INPUT: scan_code - code for key pressed
 * FUNCTION: depending on key (or keys) pressed
 */
void process_key(unsigned char scan_code)
{
    const int i = scan_code - 1;
    unsigned char key;
    switch(scan_code){
        case ENTER:
            putc_input('\n');
            term_buffer[current_terminal][term_loc[current_terminal]] = '\n';
            entered[current_terminal] = 1;
            term_loc[current_terminal] = 0;
            update_cursor();
            break;
        case DELETE:
            if(term_loc[current_terminal] > 0){
                putc_input('\b');
                term_loc[current_terminal]--;
                update_cursor();
            }
            break;
        case CTRL:
            if(ctrl_l == 2){
                buff_to_top();
                ctrl_l = 0;
            } else {
                ctrl_l = 1;
            }
            break;
        case ALT:
            alt_l = 1;
            break;
        case UNALT:
            alt_l = 0;
            break;
        case UNCTRL:
            ctrl_l = 0;
            break;
        case UNCHAR_L:
            ctrl_l = 0;
            break;
        case SHIFT_L:
            shift = 1;
            break;
        case SHIFT_R:
            shift = 1;
            break;
        case UNSHIFT_L:
            shift = 0;
            break;
        case UNSHIFT_R:
            shift = 0;
            break;
        case CAPSLOCK:
            capslock = 1 - capslock;
            break;
        case SPACE:
            if(term_loc[current_terminal] < sizeof(term_buffer)){
                putc_input(' ');
                term_buffer[current_terminal][term_loc[current_terminal]] = ' ';
                term_loc[current_terminal]++;
                update_cursor();
            }
            break;
        default:
            if(i < sizeof(keyscan) && term_loc[current_terminal] < sizeof(term_buffer)){ // printable character
                if(ctrl_l == 1){ // don't print if control is pressed
                    if(scan_code == CHAR_L)
                        buff_to_top();
                    break;
                } else if(alt_l == 1 && 59 <= scan_code && scan_code <= 61) { // check for F1 - F3 to switch terminals
                    
                    memcpy((uint32_t*)(VIDEO_MEM_ADDRESS + ((current_terminal + 1) * KiB4)), (uint32_t*)VIDEO_MEM_ADDRESS, VID_MEM_BYTES);
                    // --- SWITCH --- //
                    switch(scan_code) {
                        case 59: // terminal 1
                            current_terminal = 2;
                            break;
                        case 60: // terminal 2
                            current_terminal = 1;
                            break;
                        case 61: // terminal 3
                            current_terminal = 0;
                            break;
                    }
                    swap_terminal_mapping(current_terminal + 1);
                    update_cursor();
                    break;
                    // ---         --- //

                } else if((i >= 15 && i <= 24) || (i >= 29 && i <= 37) || (i >= 43 && i <=49)) { //alphabet
                    if((capslock== 0 && shift == 1) || (capslock == 1 && shift == 0))
                        key = shift_keyscan[i];
                    else
                        key = keyscan[i];
                    if(scan_code == CHAR_L){
                        ctrl_l = 2;
                    }
                } else if(shift == 1) //non-alphabet characters
                    key = shift_keyscan[i];
                else
                    key = keyscan[i];
                if(key == ' ') break;  // if blank character, ignore
                // print character and add to buffer
                putc_input(key);
                term_buffer[current_terminal][term_loc[current_terminal]] = key;
                term_loc[current_terminal]++;
                update_cursor();
        }
    }
}

/* buff_to_top
 *    INPUT: None
 * FUNCTION: clears screen and moves current buffer to display 
 *              on top on screen
 *           triggered with: `Ctrl` - `l` keypress combination
 */
void buff_to_top(void)
{
    int i;
    // function in lib.c, clears video ram
    clear_to_top();
    // move buffer to top
    for(i = 0; i < term_loc[current_terminal]; i++){
        putc(term_buffer[current_terminal][i]);
    }
    update_cursor();
}

/* stdout_read
 *    INPUT: see sys_read in syscalls.c
 * FUNCTION: blank handler for stdout read
 */
int32_t stdout_read (int32_t fd, void* buf, int32_t nbytes)
{
    return -1;
}

/* stdin_read
 *    INPUT: see sys_read in syscalls.c
 * FUNCTION: reads input from shell terminal to buf
 */
int32_t stdin_read (int32_t fd, void* buf, int32_t nbytes)
{
    int parent_terminal = process_term();
    if(buf == NULL)
        return -1;
    int i;
    // clear terminal buffer
    for(i = 0; i < BUFFER_W; i++){
        term_loc[parent_terminal] = 0;
        term_buffer[parent_terminal][i] = '\0';
    }
    while(!entered[parent_terminal] && term_loc[parent_terminal] < 128 && term_loc[parent_terminal] < nbytes);
    // clear buffer
    for(i = 0; i < BUFFER_W; i++)
        ((uint8_t*)buf)[i] = '\0';
    // copy buffer from term_buffer to buf
    for(i = 0; (i < BUFFER_W) && (term_buffer[parent_terminal][i] != '\n'); i++)
        ((uint8_t*)buf)[i] = term_buffer[parent_terminal][i];
    if(term_buffer[parent_terminal][i] == '\n') {
        ((uint8_t*)buf)[i] = '\n';
        i++;
    }
    
    entered[parent_terminal] = 0;
    return i;
}

/* stdout_write
 *    INPUT: see sys_write in syscalls.c
 * FUNCTION: writes buf to shell terminal
 */
int32_t stdout_write (int32_t fd, const void* buf, int32_t nbytes)
{
    if(buf == NULL)
        return -1;
    int i = 0;
    // keep printing until null character is reached
    
    while(i < nbytes) {
        if(((uint8_t*)buf)[i] == '\0')
            putc(' ');
        else
            putc(((uint8_t*)buf)[i]);
        i++;
    }
    update_cursor();
    return i;
}

/* stdin_write
 *    INPUT: see sys_write in syscalls.c
 * FUNCTION: blank handler for stdin write
 */
int32_t stdin_write (int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

/* terminal_open
 *    INPUT: see sys_open in syscalls.c
 * FUNCTION: blank handler for terminal open
 */
int32_t terminal_open (const uint8_t* filename)
{
    return -1;
}

/* terminal_close
 *    INPUT: see sys_close in syscalls.c
 * FUNCTION: blank handler for terminal close
 */
int32_t terminal_close (int32_t fd)
{
    return -1;
}
