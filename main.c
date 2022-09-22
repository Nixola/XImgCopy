#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <xcb/xcb.h>

enum error {
    ERR_USAGE = -1,
    ERR_ALLOC = -2,
    ERR_READ = -3,
    ERR_OWN = -4,
    ERR_EVENT = -5
};

enum atom {
	ATOM_CLIPBOARD,
	ATOM_UTF8_STRING,
	ATOM_IMAGE_PNG,
	ATOM_TARGETS,
	ATOM_ATOM,
	ATOM_INCR,
	ATOM_NONE,

	ATOM_N
};

struct chunk {
    struct chunk *next;
    char *data;
    size_t size;
};

struct chunk*
alloc_chunk(size_t size)
{
    struct chunk *chunk = calloc(1, sizeof(struct chunk));
    if (chunk == NULL) return NULL;
    chunk->data = calloc(1, size);
    if (chunk->data == NULL) return NULL;
    return chunk;
}

struct chunk*
free_chunk(struct chunk* chunk)
{
    free(chunk->data);
    struct chunk *next = chunk->next;
    free(chunk);
    return next;
}

xcb_atom_t atoms[ATOM_N];


xcb_window_t
create_window(xcb_connection_t *connection)
{
    /* Get the first screen */
    const xcb_setup_t      *setup  = xcb_get_setup (connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
    xcb_screen_t           *screen = iter.data;

    uint32_t mask = XCB_CW_EVENT_MASK;
    const uint32_t valwin[1] = {
		XCB_EVENT_MASK_STRUCTURE_NOTIFY        |
		XCB_EVENT_MASK_PROPERTY_CHANGE
	};

    /* Create the window */
    xcb_window_t window = xcb_generate_id (connection);
    xcb_create_window (connection,                    /* Connection              */
                       XCB_COPY_FROM_PARENT,          /* depth (same as root)    */
                       window,                        /* window Id               */
                       screen->root,                  /* parent window           */
                       -10, -10,                      /* x, y                    */
                       1, 1,                          /* width, height           */
                       0,                             /* border_width            */
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class                   */
                       screen->root_visual,           /* visual                  */
                       mask, valwin);                 /* masks, and their params */
   return window;
}

void
init_atoms(xcb_connection_t *connection)
{
	xcb_intern_atom_cookie_t requests[ATOM_N];
    xcb_intern_atom_reply_t *reply;
    requests[0] = xcb_intern_atom(connection, 0, 9, "CLIPBOARD");
    requests[1] = xcb_intern_atom(connection, 0, 11, "UTF8_STRING");
    requests[2] = xcb_intern_atom(connection, 0, 9, "image/png");
    requests[3] = xcb_intern_atom(connection, 0, 7, "TARGETS");
    requests[4] = xcb_intern_atom(connection, 0, 4, "ATOM");
    requests[5] = xcb_intern_atom(connection, 0, 4, "INCR");
    requests[6] = xcb_intern_atom(connection, 0, 4, "NONE");

    for (int i = 0; i < ATOM_N; i++) {
    	if ( (reply = xcb_intern_atom_reply(connection, requests[i], NULL)) )  {
    		atoms[i] = reply->atom;
    	}
    }
}

void
send_targets(xcb_connection_t *connection, xcb_selection_request_event_t *request)
{
	const xcb_atom_t targets[] = {
		atoms[ATOM_TARGETS],
		atoms[ATOM_UTF8_STRING],
		atoms[ATOM_IMAGE_PNG]
	};
	xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, request->requestor,
            request->property, atoms[ATOM_ATOM],
            sizeof(xcb_atom_t) * 8,
            sizeof(targets) / sizeof(xcb_atom_t), targets);
}

void
send_text(xcb_connection_t *connection, xcb_selection_request_event_t *request, char *text)
{
	xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, request->requestor,
			request->property, atoms[ATOM_UTF8_STRING],
			8,
			strlen(text), text);
}

void
send_incr(xcb_connection_t *connection, xcb_selection_request_event_t *request, uint32_t *bytes)
{
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, request->requestor,
        request->property, atoms[ATOM_INCR],
        32,
        1, bytes);
    uint32_t mask = XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes(connection, request->requestor, XCB_CW_EVENT_MASK, &mask);
}

void stop_incr(xcb_connection_t *connection, xcb_property_notify_event_t *notify)
{
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, notify->window, notify->atom, atoms[ATOM_IMAGE_PNG], 8, 0, 0);
    xcb_change_window_attributes(connection, notify->window, 0, 0);

}

void
send_chunk(xcb_connection_t *connection, xcb_property_notify_event_t *notify, struct chunk *chunk)
{
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, notify->window, notify->atom, atoms[ATOM_IMAGE_PNG], 8, chunk->size, chunk->data);
}

int
main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: cat file.png | %s URL\n", argv[0]);
        return ERR_USAGE;
	}
	char *text = argv[1];

	/* Open the connection to the X server */
    xcb_connection_t *connection = xcb_connect (NULL, NULL);

    size_t chunk_size = xcb_get_maximum_request_length(connection);
    uint32_t bytes = 0;
    struct chunk *const data = alloc_chunk(chunk_size);
    if (data == NULL) return ERR_ALLOC;
    struct chunk *data_head = data;
    freopen(NULL, "rb", stdin);
    while (1) {
        data_head->size = fread(data_head->data, sizeof(char), chunk_size, stdin);
        if (ferror(stdin)) {
            return ERR_READ;
        }
        bytes += data_head->size;
        if (feof(stdin)) break;
        data_head->next = alloc_chunk(chunk_size);
        if (data_head->next == NULL) {
            return ERR_ALLOC;
        }
        data_head = data_head->next;
    }
    data_head = data;
    xcb_window_t window = create_window(connection);

    init_atoms(connection);

    xcb_set_selection_owner(connection, window, atoms[ATOM_CLIPBOARD], XCB_CURRENT_TIME);
    xcb_flush(connection);

    if (xcb_get_selection_owner_reply(connection, xcb_get_selection_owner(connection, atoms[ATOM_CLIPBOARD]), NULL)->owner != window) {
        return ERR_OWN;
    }

    xcb_generic_event_t *event;
    int incr = 0;
    while ( (event = xcb_wait_for_event (connection)) ) {
        switch (event->response_type & ~0x80) {
        case XCB_DESTROY_NOTIFY: {
            xcb_destroy_notify_event_t *evt = (xcb_destroy_notify_event_t *)event;
            if (evt->window == window) {
        		// You're being closed
                free(event); /* XCB: Do not use custom allocators */
                return 0;
            }
        	break;
        }
        case XCB_SELECTION_CLEAR: {
            // Something else was copied; your job is done
            free(event);
            return 0;
	        break;
        }
        case XCB_SELECTION_REQUEST: {
        	// You're being asked for the clipboard 
        	xcb_selection_request_event_t *request = (xcb_selection_request_event_t *) event;
            if (request->selection != atoms[ATOM_CLIPBOARD]) {
            	break;
            }
        	xcb_atom_t target = request->target;
        	xcb_selection_notify_event_t reply;

        	reply.response_type = XCB_SELECTION_NOTIFY;
            reply.requestor = request->requestor;
            reply.selection = request->selection;
            reply.target = request->target;
            reply.time = request->time;
            reply.property = request->property == atoms[ATOM_NONE] ? request->target : request->property;

        	if (target == atoms[ATOM_TARGETS]) {
        		send_targets(connection, request);
        	} else if (target == atoms[ATOM_UTF8_STRING]) {
        		send_text(connection, request, text);
        	} else if (target == atoms[ATOM_IMAGE_PNG]) {
        		// Send INCR; you have to handle this whole thing now, I hope you're happy
        		// TODO: do it
                send_incr(connection, request, &bytes);
                incr = 1;
        	} else {
        		// Heck if I know what you want; explicitly refuse
        		reply.property = XCB_ATOM_NONE;
        	}
        	xcb_send_event(connection, 0, request->requestor, XCB_EVENT_MASK_PROPERTY_CHANGE, (const char *) &reply);
        	xcb_flush(connection);
            break;
        }
        case XCB_PROPERTY_NOTIFY: {
            xcb_property_notify_event_t *notify = (xcb_property_notify_event_t *) event;
            if (notify->state != XCB_PROPERTY_DELETE) break;
            if (incr == 0) break;
            if (data_head == NULL) {
                stop_incr(connection, notify);
                xcb_flush(connection);
                data_head = data;
                incr = 0;
                break;
            }
            send_chunk(connection, notify, data_head);
            xcb_flush(connection);
            data_head = data_head->next;
            break;
        }
        default:
            // Don't know, don't care
            break;
        }

        free (event);
    }

	return ERR_EVENT;
}