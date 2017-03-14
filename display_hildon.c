/***************************************************************
*  display_hildon.c                                #######     *
*  chaos-eng(c) int13h 2005-2010                   #.....#     *
*                                                  #..@..#     *
*  draw routines for Hildon                        #.....#     *
*                                                  #######     *
*                                                              *
***************************************************************/

/*
############
#..Globals.#
############
*/
#include "globals.h"
#include "structs.h"
#include "main.h"
#include "log.h"
#include "chaos.h"
#include "display_common.h"
#include "display_hildon.h"
#include "input_hildon.h"

extern world                myworld;
extern player               myplayers[MAX_PLAYERS];
extern spells               myspells[MAX_SPELLS];
extern monster              mymonsters[MAX_MONSTERS];
extern tree                 mytrees[MAX_TREES];
extern wall                 mywalls[MAX_WALLS];
extern blob                 myblobs[MAX_BLOBS];
extern magic_special        mymagic_special[MAX_MAGIC_SPECIAL];
extern magic_upgrade        mymagic_upgrade[MAX_MAGIC_UPGRADE];
extern magic_balance        mymagic_balance[MAX_MAGIC_BALANCE];
extern magic_spell_attrib   mymagic_spell_attrib[MAX_MAGIC_SPELL_ATTRIB];
extern magic_ranged         mymagic_ranged[MAX_MAGIC_RANGED];

extern char infobar_text[255];
extern bool beepmsg;

extern char log_message[LOGMSGLEN];

extern char input_buffer[255];

extern int arenas[MAX_ARENAS][2];

extern history_log myhistory[MAX_HISTORY];
extern int history_count;

extern chat_log mychat[MAX_CHAT];
extern int chat_count;

static char *CE_COLORS_HILDON[] = {
    [CE_C_LGREY] = "A/A/A",
    [CE_C_GREY] = "7/7/7",
    [CE_C_DGREY] = "3/3/3",

    [CE_C_LVIOLET] = "F/0/C",
    [CE_C_VIOLET] = "B/0/F",
    [CE_C_DVIOLET] = "6/0/C",

    [CE_C_LWHITE] = "F/F/F",
    [CE_C_WHITE] = "F/F/F",
    [CE_C_DWHITE] = "A/A/A",

    [CE_C_LBLUE] = "0/0/8",
    [CE_C_BLUE] = "5/5/F",
    [CE_C_DBLUE] = "A/A/F",

    [CE_C_LGREEN] = "0/8/0",
    [CE_C_GREEN] = "0/F/0",
    [CE_C_DGREEN] = "9/F/9",

    [CE_C_LRED] = "F/6/6",
    [CE_C_RED] = "F/0/0",
    [CE_C_DRED] = "F/9/9",

    [CE_C_LORANGE] = "F/9/3",
    [CE_C_ORANGE] = "F/6/0",
    [CE_C_DORANGE] = "A/3/0",

    [CE_C_LYELLOW] = "F/E/7",
    [CE_C_YELLOW] = "F/E/0",
    [CE_C_DYELLOW] = "A/8/0",

    [CE_C_LBROWN] = "F/9/6",
    [CE_C_BROWN] = "9/6/3",
    [CE_C_DBROWN] = "6/3/0",

    [CE_C_LPURPLE] = "9/9/E",
    [CE_C_PURPLE] = "9/3/C",
    [CE_C_DPURPLE] = "6/0/9",

    [CE_C_LCYAN] = "6/F/F",
    [CE_C_CYAN] = "3/A/A",
    [CE_C_DCYAN] = "0/9/9"
};

static int CE_MULTI_COLORS_HILDON[][6] = {
    [CE_C_MULTI1] = {CE_C_YELLOW,CE_C_VIOLET,CE_C_BLUE,CE_C_GREEN,CE_C_RED,CE_C_CYAN},
    [CE_C_MULTI2] = {CE_C_RED,CE_C_LRED,CE_C_DRED,CE_C_RED,CE_C_LRED,CE_C_DRED},
    [CE_C_MULTI3] = {CE_C_GREEN,CE_C_LGREEN,CE_C_DGREEN,CE_C_GREEN,CE_C_LGREEN,CE_C_DGREEN}
};

HildonProgram *program;
HildonWindow *window;

GtkWidget *vbox,*drawing_area;
static GdkPixmap *pixmap=NULL;

/*
######################
#..configure_event().#
######################
*/
static gboolean configure_event(GtkWidget *widget,GdkEventConfigure *event)
{
    if(pixmap)
        g_object_unref (pixmap);

    pixmap=gdk_pixmap_new(widget->window,
        widget->allocation.width,
        widget->allocation.height,
        -1);
    gdk_draw_rectangle(pixmap,
        widget->style->black_gc,
        TRUE,
        0, 0,
        widget->allocation.width,
        widget->allocation.height);

    return TRUE;
}

/*
###################
#..expose_event().#
###################
*/
static gboolean expose_event(GtkWidget *widget,GdkEventExpose *event)
{
    gdk_draw_drawable (widget->window,
        widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
        pixmap,
        event->area.x, event->area.y,
        event->area.x, event->area.y,
        event->area.width, event->area.height);

    return FALSE;
}

/*
##################
#..init_hildon().#
##################
*/
void init_hildon (void)
{
    int argc=0;
    char **argv;

    gtk_init(&argc,&argv);

    /* Create the hildon program and setup the title */
    program=HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("Chaos");

    /* Create HildonWindow and set it to HildonProgram */
    window=HILDON_WINDOW(hildon_window_new());
    hildon_program_add_window(program,window);

    /* Create Display Area Widget */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show (vbox);

    /* Create The Drawing Area */
    drawing_area=gtk_drawing_area_new ();
    gtk_widget_set_size_request (GTK_WIDGET(drawing_area), 200, 200);
    gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

    gtk_widget_show (drawing_area);

    /* Signals used to handle backing pixmap */
    g_signal_connect(G_OBJECT (drawing_area), "expose_event",
        G_CALLBACK(expose_event), NULL);
    g_signal_connect(G_OBJECT (drawing_area),"configure_event",
        G_CALLBACK(configure_event), NULL);

    gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
        | GDK_LEAVE_NOTIFY_MASK
        | GDK_BUTTON_PRESS_MASK
        | GDK_POINTER_MOTION_MASK
        | GDK_POINTER_MOTION_HINT_MASK);

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(window));

    /* Connect signal to X in the upper corner */
    g_signal_connect(G_OBJECT(window), "delete_event",
      G_CALLBACK(shutdown_chaos),NULL);

    /* Connect Keyboard Input Signals to input_hildon.c */
    g_signal_connect(G_OBJECT(window), "key_press_event",
        G_CALLBACK(gethildonkey), NULL);

    doevents_hildon();
}

/*
######################
#..shutdown_hildon().#
######################
*/
void shutdown_hildon(void)
{
    doevents_hildon();
    gtk_main_quit();
}

/*
######################
#..doevents_hildon().#
######################
*/
void doevents_hildon (void)
{
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

/*
##########################
#..enable_color_hildon().#
##########################
*/
static void enable_color_hildon (int color)
{
    return;
}

/*
###########################
#..disable_color_hildon().#
###########################
*/
static void disable_color_hildon (void)
{
    return;
}

/*
##########################
#..cleanbuffers_hildon().#
##########################
*/
static void cleanbuffers_hildon (void)
{

}

/*
##########################
#..flushbuffers_hildon().#
##########################
*/
static void flushbuffers_hildon (void)
{
    printf("flush!\n");
    gtk_widget_queue_draw(drawing_area);
}

/*
#########################
#..drawhistory_hildon().#
#########################
*/
void drawhistory_hildon (void)
{

}

/*
######################
#..drawchat_hildon().#
######################
*/
void drawchat_hildon (void)
{

}

/*
#######################
#..drawscene_hildon().#
#######################
*/
void drawscene_hildon (void)
{
    PangoLayout *layout = gtk_widget_create_pango_layout(drawing_area, 0);
    GdkGC *gc = drawing_area->style->white_gc;

    pango_layout_set_text(layout, "moo", 3);
    gdk_draw_layout(drawing_area->window, gc, 50, 50, layout);

    printf("drawscene!\n");

    flushbuffers_hildon();
}

/*
########################
#..drawserver_hildon().#
########################
*/
void drawserver_hildon (void)
{
    cleanbuffers_hildon();
    flushbuffers_hildon();
}
