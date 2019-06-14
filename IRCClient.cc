#include <stdio.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <time.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int port;
char * host;
char * sport;
char * userName;
char * password;
char * argument;
GtkWidget * table;
GtkWidget * window;
GtkWidget * listRoom;
GtkWidget * listUser;
GtkWidget * labelName;
GtkWidget * labelPass;
GtkWidget * tree_view;
GtkWidget * chatMessages;
GtkListStore * list_rooms;
GtkListStore * list_users;
GtkWidget * entryEnterRoom;
GtkWidget * entryLeaveRoom;
GtkWidget * entryCreateRoom;
GtkWidget * labelRoomNameEntered;

static char buffer[256];

char * response = (char *)malloc(100 * sizeof(char));
char * newMessage = (char *)malloc(200 * sizeof(char));
const char * newRoom = (char *)malloc(100 * sizeof(char));
const char * newUser = (char *)malloc(100 * sizeof(char));
const char * newPassword = (char *)malloc(100 * sizeof(char));



int open_client_socket(char * host, int port) 
{
    struct sockaddr_in socketAddress;
    memset((char *)&socketAddress,0,sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons((u_short)port);
    struct    hostent    *ptrh = gethostbyname(host);
    if ( ptrh == NULL ) 
    {
        perror("gethostbyname");
        exit(1);
    }
    memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
    struct    protoent *ptrp = getprotobyname("tcp");
    if ( ptrp == NULL ) {
    perror("getprotobyname");
    exit(1);
    }
    int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sock < 0) 
    {
    perror("socket");
    exit(1);
    }
    if (connect(sock, (struct sockaddr *)&socketAddress,
        sizeof(socketAddress)) < 0) {
    perror("connect");
    exit(1);
    }
    return sock;
}

int sendCommand(char * host, int port, char * command, const char * userName,
    const char * password, const char * argument, char * message, char * 
response)
{
   
    int sock = open_client_socket(host, port);

    if (sock < 0) 
    {
    return 0;
    }

    write(sock, command, strlen(command));
    write(sock, " ", 1);
    write(sock, userName, strlen(userName));
    write(sock, " ", 1);
    write(sock, password, strlen(password));
    write(sock, " ", 1);
    write(sock, argument, strlen(argument));
    write(sock, " ", 1);
    write(sock, message, strlen(message));

    write(sock, "\r\n",2);

    write(1, command, strlen(command));
    write(1, " ", 1);
    write(1, userName, strlen(userName));
    write(1, " ", 1);
    write(1, password, strlen(password));
    write(1, " ", 1);
    write(1, argument, strlen(argument));
    write(1, " ", 1);
    write(1, message, strlen(message));
    write(1, "\r\n",2);

        int n = 0;
        int len = 0;
        while ((n=read(sock, response+len, (1024*10) - len))>0) {
                len += n;
        }
        response[len]=0;
        printf("response:\n%s\n", response);

    close(sock);

    return 1;
}

static GtkWidget * create_list( const char * titleColumn, GtkListStore * model ) 
{
    GtkWidget *scrolled_window;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    tree_view = gtk_tree_view_new ();

    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view);

     
    cell = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (titleColumn, cell,"text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), GTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}

void enter_room_by_choice(GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * col, gpointer userdata) 
{
    GtkTreeIter iter;
    GtkTreeModel * mod = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_get_iter(mod, &iter, path)) 
    {
        gchar * rname;
        gtk_tree_model_get(mod, &iter, 0, &rname, -1);
        newRoom = strdup(rname);

        sendCommand(host, port, "ENTER-ROOM", newUser, newPassword, newRoom, "NULL", response);

        if (strcmp(response, "OK\r\n") == 0) 
	{
            g_print("System: %s is entering room %s.", newUser, newRoom);
            sendCommand(host, port, "SEND-MESSAGE", newUser, newPassword, newRoom, "ENTERED ROOM ", response);
        }   

        g_free(rname);
    }

    gtk_label_set_markup(GTK_LABEL(labelRoomNameEntered), newRoom);   
}

static GtkWidget * get_list_rooms() {
    GtkTreeIter iter;
    GtkListStore * sign_list_rooms;
    tree_view = gtk_tree_view_new ();

    create_list("Users In Room", sign_list_rooms);
    
    sendCommand(host, port, "LIST-ROOMS", newUser, newPassword, strdup("NULL"), "NULL", response);
    if (response != NULL) 
    {
        gtk_widget_hide (listRoom);   
        sign_list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
        char * name_of_room = strtok(response, "\r\n");
        
	while(name_of_room != NULL) 
	{
            gchar *message = g_strdup_printf ("%s", name_of_room);
            gtk_list_store_append (GTK_LIST_STORE (sign_list_rooms), &iter);

            gtk_list_store_set (GTK_LIST_STORE (sign_list_rooms), &iter,0, message,-1);
            g_free (message);

            name_of_room = strtok(NULL, "\r\n");
        }

        listRoom = create_list ("Rooms", sign_list_rooms);
        gtk_table_attach_defaults (GTK_TABLE (table), listRoom, 3, 6, 0, 3);
        gtk_widget_show (listRoom);
    }
       g_signal_connect(tree_view, "ROW-ACTIVATED", G_CALLBACK(enter_room_by_choice), NULL);
}

void get_list_users() {
    GtkTreeIter iter;
    GtkListStore * sign_list_users;

    sendCommand(host, port, "GET-USERS-IN-ROOM", newUser, newPassword, newRoom, strdup("NULL"), response);
    if (strcmp(response, "DENIED\r\n") != 0) 
    {
        gtk_widget_hide (listUser);   
        sign_list_users = gtk_list_store_new (1, G_TYPE_STRING);
        char * userNameInRoom;
        userNameInRoom = strtok(response, "\r\n");
        while(userNameInRoom != NULL) {
            gchar *msg = g_strdup_printf ("%s", userNameInRoom);
            gtk_list_store_append (GTK_LIST_STORE (sign_list_users), 
&iter);
            gtk_list_store_set (GTK_LIST_STORE (sign_list_users),
                            &iter,
                                0, msg,
                            -1);
            g_free (msg);
            userNameInRoom = strtok(NULL, "\r\n");
        }
        listUser = create_list ("Users In Room", sign_list_users);
        gtk_table_attach_defaults (GTK_TABLE (table), listUser, 0, 3, 0, 
3);
        gtk_widget_show (listUser);   
    }
    else if (strcmp(response, "ERROR (Wrong password)\r\n") == 0) {
        sign_list_users = gtk_list_store_new (1, G_TYPE_STRING);
        gchar *msg = g_strdup_printf ("No User In Room");
        gtk_list_store_append (GTK_LIST_STORE (sign_list_users), &iter);
        gtk_list_store_set (GTK_LIST_STORE (sign_list_users),
                            &iter,
                                0, msg,
                            -1);
        listUser = create_list ("Users In Room", sign_list_users);
        gtk_table_attach_defaults (GTK_TABLE (table), listUser, 0, 3, 0, 
3);
        gtk_widget_show (listUser);   
    }
    else {
        sign_list_users = gtk_list_store_new (1, G_TYPE_STRING);
        gchar *msg = g_strdup_printf ("No User In Room");
        gtk_list_store_append (GTK_LIST_STORE (sign_list_users), &iter);
        gtk_list_store_set (GTK_LIST_STORE (sign_list_users),
                            &iter,
                                0, msg,
                            -1);
        listUser = create_list ("Users In Room", sign_list_users);
        gtk_table_attach_defaults (GTK_TABLE (table), listUser, 0, 3, 0, 
3);
        gtk_widget_show (listUser);   
    }
}

static void insert_text( GtkTextBuffer *buffer, const char * initialText ) 
{
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
    gtk_text_buffer_insert (buffer, &iter, 
initialText,strlen(initialText));
}
     
static GtkWidget * create_text(char * initialText) 
{
    GtkWidget * view = gtk_text_view_new ();
    GtkTextBuffer * buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

    GtkWidget * scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), 
GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER (scrolled_window), view);
    gtk_text_buffer_set_text (buffer, initialText, strlen(initialText));

    PangoFontDescription * font_desc = pango_font_description_from_string ("Serif 15");
    gtk_widget_modify_font (view, font_desc);
    pango_font_description_free (font_desc);

    gtk_widget_show_all (scrolled_window);

    return scrolled_window;
}

void get_message() {
    gtk_widget_hide(chatMessages);
    if (strcmp(newRoom, "NULL") != 0)
        sendCommand(host, port, "GET-MESSAGES", newUser, newPassword, "0", strdup(newRoom), response);
    if (strcmp(response, "DENIED\r\n") != 0)
        chatMessages = create_text (response);
    else
        chatMessages = create_text("You are not in any chat room.\r\n");
    gtk_table_attach_defaults (GTK_TABLE (table), chatMessages, 0, 6, 3, 7);
}

static gboolean on_expose_event(GtkWidget *widget, GdkEventExpose *event, 
gpointer data) 
{
    cairo_t *cr = gdk_cairo_create(widget->window);

    cairo_move_to(cr, 30, 30);
    cairo_show_text(cr, buffer);

    cairo_destroy(cr);

    return FALSE;
}

static gboolean time_handler(GtkWidget *widget) 
{
    if (widget->window == NULL)
     {   return FALSE;
     }

    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);

    strftime(buffer, 256, "%T", local_time);

    gtk_widget_queue_draw(widget);

    return TRUE;
}

static gboolean get_update(GtkWidget *widget) {
    if (widget->window == NULL)
    {    return FALSE;
     }

    time_t current_time;
    struct tm *local_time;

    current_time = time(NULL);
    local_time = localtime(&current_time);

    strftime(buffer, 256, "%T", local_time);

    gtk_widget_queue_draw(widget);
    get_list_rooms();
    get_list_users();
    get_message();

    return TRUE;
}

void update_timely() 
{
    g_timeout_add(1000, (GSourceFunc) get_update, (gpointer) window);
}

void create_new_room() 
{
    newRoom = gtk_entry_get_text(GTK_ENTRY(entryCreateRoom));
    sendCommand(host, port, "CREATE-ROOM", newUser, newPassword, newRoom, 
"NULL", response);
}

void create_room() {
    GtkWidget *window_create_room;
    window_create_room = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window_create_room), "Create Room");
    g_signal_connect (window_create_room, "destroy",
                G_CALLBACK (gtk_window_iconify), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window_create_room), 
10);
    gtk_widget_set_size_request (GTK_WIDGET (window_create_room), 350, 
300);

    GtkWidget *table = gtk_table_new (12, 7, TRUE);
    gtk_container_add (GTK_CONTAINER (window_create_room), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 5);
    gtk_table_set_col_spacings(GTK_TABLE (table), 5);
    gtk_widget_show (table);

    GtkWidget *labelCreateRoom = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(labelCreateRoom), "<big>Create Room</big>");
    gtk_table_attach_defaults(GTK_TABLE (table), labelCreateRoom, 1, 6, 0, 1);
    gtk_widget_show (labelCreateRoom);

    GtkWidget *labelRoom = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(labelRoom), "<big>Room Name</big>");
    gtk_table_attach_defaults(GTK_TABLE (table), labelRoom, 1, 6, 3, 4);
    gtk_widget_show (labelRoom);

    entryCreateRoom = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entryCreateRoom), " ");
    gtk_table_attach_defaults(GTK_TABLE (table), entryCreateRoom, 1, 6, 5, 
6);
    gtk_widget_show (entryCreateRoom);

    GtkWidget *continue_button = gtk_button_new_with_label ("Continue");
    gtk_table_attach_defaults(GTK_TABLE (table), continue_button, 1, 6, 9, 
10);
    gtk_widget_show (continue_button);
    g_signal_connect(continue_button, "clicked", 
G_CALLBACK(create_new_room), NULL);
    g_signal_connect_swapped(continue_button, "clicked", (GCallback) 
gtk_widget_hide, window_create_room);

    gtk_widget_show (window_create_room);
}

void enter_new_room() {
    newRoom = gtk_entry_get_text(GTK_ENTRY(entryEnterRoom));
    
    sendCommand(host, port, "ENTER-ROOM", newUser, newPassword, newRoom, "NULL", response);
    sendCommand(host, port, "SEND-MESSAGE", newUser, newPassword, newRoom, 
"ENTERED ROOM", response);
}

void enter_room() {
    GtkWidget *window_enter_room;
    window_enter_room = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window_enter_room), "Enter Room");
    g_signal_connect (window_enter_room, "destroy",
                G_CALLBACK (gtk_window_iconify), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window_enter_room), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window_enter_room), 350, 300);

    GtkWidget *table = gtk_table_new (12, 7, TRUE);
    gtk_container_add (GTK_CONTAINER (window_enter_room), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 5);
    gtk_table_set_col_spacings(GTK_TABLE (table), 5);
    gtk_widget_show (table);

    GtkWidget *labelEnterRoom = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(labelEnterRoom), "<big>Enter Room</big>");
    gtk_table_attach_defaults(GTK_TABLE (table), labelEnterRoom, 1, 6, 0, 1);
    gtk_widget_show (labelEnterRoom);

    GtkWidget *labelRoom = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(labelRoom), "<big>Room Name</big>");
    gtk_table_attach_defaults(GTK_TABLE (table), labelRoom, 1, 6, 3, 4);
    gtk_widget_show (labelRoom);

    entryEnterRoom = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entryEnterRoom), "name");
    gtk_table_attach_defaults(GTK_TABLE (table), entryEnterRoom, 1, 6, 5, 
6);
    gtk_widget_show (entryEnterRoom);

    GtkWidget *continue_button = gtk_button_new_with_label ("Continue");
    gtk_table_attach_defaults(GTK_TABLE (table), continue_button, 1, 6, 9, 
10);
    gtk_widget_show (continue_button);
    g_signal_connect(continue_button, "clicked", 
G_CALLBACK(enter_new_room), NULL);
    g_signal_connect_swapped(continue_button, "clicked", (GCallback) 
gtk_widget_hide, window_enter_room);

    gtk_widget_show (window_enter_room);
}

void leave_room() {
         sendCommand(host, port, "SEND-MESSAGE", newUser, newPassword, newRoom, "IS LEAVING ROOM", response);
    sendCommand(host, port, "LEAVE-ROOM", newUser, newPassword, newRoom, 
"NULL", response);
}

void add_new_user() 
{

    char * response = (char *)malloc(100 * sizeof(char));
    sendCommand(host, port, "ADD-USER", newUser, newPassword, "NULL", "NULL", response);

    if (strcmp(response, "DENIED\r\n") == 0)
       { response = strdup("Log In Successful!");
    }
    else
        response = strdup("Sign Up Successful!");
    update_timely();
}

void signup() {
    GtkWidget *window_sign_up;
    window_sign_up = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window_sign_up), "Sign Up");
    g_signal_connect (window_sign_up, "destroy",
                G_CALLBACK (gtk_window_iconify), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window_sign_up), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window_sign_up), 350, 300);

    GtkWidget *table = gtk_table_new (10, 7, TRUE);
    gtk_container_add (GTK_CONTAINER (window_sign_up), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 5);
    gtk_table_set_col_spacings(GTK_TABLE (table), 5);
    gtk_widget_show (table);

    GtkWidget *labelSignUp = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(labelSignUp), "<big>Sign up / Log In</big>");
    gtk_table_attach_defaults(GTK_TABLE (table), labelSignUp, 1, 6, 0, 1);
    gtk_widget_show (labelSignUp);

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<big>User Name</big>");
    gtk_table_attach_defaults(GTK_TABLE (table), label, 1, 6, 2, 3);
    gtk_widget_show (label);

    GtkWidget * entryName = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entryName), "uname");
    gtk_table_attach_defaults(GTK_TABLE (table), entryName, 1, 6, 3, 4);
    gtk_widget_show (entryName);

    GtkWidget *labelPassWord = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(labelPassWord), "<big>Password</big>");
    gtk_table_attach_defaults(GTK_TABLE (table), labelPassWord, 1, 6, 5, 6);
    gtk_widget_show (labelPassWord);

    GtkWidget * entryPassword = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entryPassword), "pass");
    gtk_table_attach_defaults(GTK_TABLE (table), entryPassword, 1, 6, 6, 7);
    gtk_widget_show (entryPassword);

    newUser = gtk_entry_get_text(GTK_ENTRY(entryName));
    newPassword = gtk_entry_get_text(GTK_ENTRY(entryPassword));

    GtkWidget *continue_button = gtk_button_new_with_label ("Continue");
    gtk_table_attach_defaults(GTK_TABLE (table), continue_button, 1, 6, 8, 9);
    gtk_widget_show (continue_button);
    g_signal_connect(continue_button, "clicked", G_CALLBACK(add_new_user), NULL);

    g_signal_connect_swapped(continue_button, "clicked", (GCallback) 
   gtk_widget_hide, window_sign_up);

    gtk_widget_show (window_sign_up);
}

void send_message(GtkWidget * entry) 
{
    const char * send_msg = gtk_entry_get_text(GTK_ENTRY(entry));
    
    newMessage = strdup(send_msg);

    if (strcmp(strdup(newRoom), "NULL") != 0)
        sendCommand(host, port, "SEND-MESSAGE", newUser, newPassword, newRoom,newMessage, response);
}

void update_list_rooms() 
{
    GtkTreeIter iter;
    int b;

    for (b = 0; b < 3; b++) {
        gchar *msg = g_strdup_printf ("Room %d", b);
        gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
        gtk_list_store_set (GTK_LIST_STORE (list_rooms),&iter,0, msg,-1);
             g_free (msg);
    }
}

void update_list_users() 
{
    GtkTreeIter iter;
    

    for (int a = 0; a < 3; a = a+1) {
        gchar *msg = g_strdup_printf ("User %d", a);
        gtk_list_store_append (GTK_LIST_STORE (list_users), &iter);
        gtk_list_store_set (GTK_LIST_STORE (list_users),&iter,0, msg,-1);
             g_free (msg);
    }
}

int main(int argc, char *argv[] ) 
{

    char * command;
   
    if (argc < 3) 
    {
        printf("Usage: test-talk-server host port\n");
             exit(1);
    }

    host = argv[1];
    sport = argv[2];

    sscanf(sport, "%d", &port);

    newRoom = "NULL";

    gtk_init (&argc, &argv);
     
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Internet Relay Chat");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 1000 , 600);

    table = gtk_table_new (11, 6, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 5);
    gtk_table_set_col_spacings(GTK_TABLE (table), 5);
    gtk_widget_show (table);

    list_users = gtk_list_store_new (1, G_TYPE_STRING);
    update_list_users();
    listUser = create_list ("Users In Room", list_users);
    gtk_table_attach_defaults (GTK_TABLE (table), listUser, 0, 3, 0, 3);
    gtk_widget_show (listUser);

    list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
    update_list_rooms();
    listRoom = create_list ("Rooms", list_rooms);
    gtk_table_attach_defaults (GTK_TABLE (table), listRoom, 3, 6, 0, 3);
    gtk_widget_show (listRoom);   

    chatMessages = create_text ("Sign up or Login to begin\n");
    gtk_table_attach_defaults (GTK_TABLE (table), chatMessages, 0, 6, 3, 7);
    gtk_widget_show (chatMessages);

    GtkWidget *create_room_button = gtk_button_new_with_label ("Create Room");
    gtk_table_attach_defaults(GTK_TABLE (table), create_room_button, 0, 2, 7, 8);
    gtk_widget_show (create_room_button);
    g_signal_connect(create_room_button, "clicked", G_CALLBACK(create_room), NULL);

    GtkWidget *enter_room_button = gtk_button_new_with_label ("Enter Room");
    gtk_table_attach_defaults(GTK_TABLE (table), enter_room_button, 2, 4, 7, 8);
    gtk_widget_show (enter_room_button);
    g_signal_connect(enter_room_button, "clicked", G_CALLBACK(enter_room), NULL);

    GtkWidget *leave_room_button = gtk_button_new_with_label ("Leave Room");
    gtk_table_attach_defaults(GTK_TABLE (table), leave_room_button, 4, 7, 7, 8);
    gtk_widget_show (leave_room_button);
    g_signal_connect(leave_room_button, "clicked", G_CALLBACK(leave_room), NULL);

    GtkWidget * entryMessage = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entryMessage), "New Message");
    gtk_table_attach_defaults(GTK_TABLE (table), entryMessage, 0, 6, 8, 9);
    gtk_widget_show (entryMessage);

    GtkWidget *send_button = gtk_button_new_with_label ("Send");
    gtk_table_attach_defaults(GTK_TABLE (table), send_button, 0, 6, 9, 13);
    const char * messageContent; 
     messageContent = gtk_entry_get_text(GTK_ENTRY(entryMessage));
    g_signal_connect_swapped(send_button, "clicked", 
    G_CALLBACK(send_message), entryMessage);
    g_signal_connect(send_button, "clicked", G_CALLBACK(get_message), NULL);
    gtk_widget_show (send_button);
   
   

   
    signup();
    gtk_widget_show (table);
    gtk_widget_show (window);

    gtk_main ();
    return 0;
}

