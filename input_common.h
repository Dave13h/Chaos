/***************************************************************
*  input_common.h                                  #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  Common input headers                            #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

/*
    -==========================-
     Chaos Eng internal Keysym :
    -==========================-

    This is our interpretation of the keynames to make it easy
    for us to use any 'toolkit/front end' generically.
    CE_KEYS are for special keys.
*/
typedef enum {
    CE_RETURN       = 13000,
    CE_ESCAPE       = 13001,
    CE_BACKSPACE    = 13002,

    CE_INSERT       = 13003,
    CE_DELETE       = 13004,
    CE_HOME         = 13005,
    CE_END          = 13006,
    CE_PGUP         = 13007,
    CE_PGDN         = 13008,

    CE_UP           = 13009,    /* Cursor keys */
    CE_DOWN         = 13010,
    CE_LEFT         = 13011,
    CE_RIGHT        = 13012,
    CE_UPLEFT       = 13013,
    CE_UPRIGHT      = 13014,
    CE_DOWNLEFT     = 13015,
    CE_DOWNRIGHT    = 13016,

    CE_F1           = 13017,    /* Function Keys */
    CE_F2           = 13018,
    CE_F3           = 13019,
    CE_F4           = 13020,
    CE_F5           = 13021,
    CE_F6           = 13022,
    CE_F7           = 13023,
    CE_F8           = 13024,
    CE_F9           = 13025,
    CE_F10          = 13026,
    CE_F11          = 13027,
    CE_F12          = 13028,

    CE_ACTION       = 13029,
    CE_TAB          = 13030

} CE_KEYS;

/*
###############
#..Prototypes.#
###############
*/
void init_input(void);
void check_input(void);
void empty_buffer(void);
bool lock_cursor(void);
bool move_cursor(int key);
bool process_input_common(int key);
bool valid_input(char input[256]);
