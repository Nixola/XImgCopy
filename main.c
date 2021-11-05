#include <stdio.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>

Atom
send_no(Display *dpy, XSelectionRequestEvent *sev)
{
    char *an;

    an = XGetAtomName(dpy, sev->target);
    printf("Denying request of type '%s'\n", an);
    if (an)
        XFree(an);

    return sev->property;
}
//*
Atom
send_targets(Display *dpy, XSelectionRequestEvent *sev, Atom type)
{
    char *an;
    const Atom targets[] = {
        XInternAtom(dpy, "TARGETS", False),
        //XInternAtom(dpy, "MULTIPLE", False),
        XInternAtom(dpy, "UTF8_STRING", False),
        XInternAtom(dpy, "image/png", False)
    };

    an = XGetAtomName(dpy, sev->property);
    printf("Sending data to window 0x%lx, property '%s' [targets]\n", sev->requestor, an);
    if (an) {
        XFree(an);
    }
    XChangeProperty(dpy, sev->requestor, sev->property, type, 32, PropModeReplace,
                    (unsigned char*) targets, sizeof(targets) / sizeof(Atom));

    return sev->property;
}
//*/
Atom
send_img(Display *dpy, XSelectionRequestEvent *sev, Atom type, char *data, size_t dataLength)
{
    char *an, *ty;

    an = XGetAtomName(dpy, sev->property);
    ty = XGetAtomName(dpy, sev->target);
    printf("Sending data to window 0x%lx, property '%s' [%s]\n", sev->requestor, an, ty);
    if (an) {
        XFree(an);
    }
    if (ty) {
        XFree(ty);
    }
    XChangeProperty(dpy, sev->requestor, sev->property, type, 8, PropModeReplace,
                    (unsigned char *) data, dataLength);
    return sev->property;
}

int
main(int argC, char** argV)
{
    const int maxSize = 1024*1024;
    Display *dpy;
    Window owner, root;
    int screen;
    Atom sel, utf8, png, tgts, atom;
    XEvent ev;
    XSelectionRequestEvent *sev;
    char img[maxSize];
    char *url;
    size_t bytes;

    if (argC != 2) {
        printf("Usage: cat file.png | %s URL\n", argV[0]);
        return -1;
    }

    url = argV[1];
    freopen(NULL, "rb", stdin);
    bytes = fread(img, sizeof(char), maxSize - 1, stdin);
    if (ferror(stdin)) {
        printf("Error when reading image from stdin.\n");
        return -2;
    }
    if (!feof(stdin)) {
        printf("It's not over, is it? (files >= %d KiB aren't supported yet)\n", maxSize / 1024);
        return -3;
    }
    printf("Read %lu bytes.\n", bytes);
    img[bytes] = '\0';

    dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        fprintf(stderr, "Could not open X display\n");
        return 1;
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    /* We need a window to receive messages from other clients. */
    owner = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);
    XSelectInput(dpy, owner, SelectionClear | SelectionRequest);

    sel = XInternAtom(dpy, "CLIPBOARD", False);
    utf8 = XInternAtom(dpy, "UTF8_STRING", False);
    png = XInternAtom(dpy, "image/png", False);
    tgts = XInternAtom(dpy, "TARGETS", False);
    atom = XInternAtom(dpy, "ATOM", False);

    /* Claim ownership of the clipboard. */
    XSetSelectionOwner(dpy, sel, owner, CurrentTime);

    for (;;)
    {
        Atom prop;
        XSelectionEvent ssev;
        XNextEvent(dpy, &ev);
        switch (ev.type)
        {
            case SelectionClear:
                printf("Lost selection ownership\n");
                return 1;
                break;
            case SelectionRequest:
                sev = (XSelectionRequestEvent*)&ev.xselectionrequest;
                printf("Requestor: 0x%lx\n", sev->requestor);
                prop = None;
                /* Property is set to None by "obsolete" clients. */
                if ((sev->target != utf8 && sev->target != png && sev->target != tgts) || sev->property == None) {
                    send_no(dpy, sev);
                } else if (sev->target == utf8) {
                    prop = send_img(dpy, sev, utf8, url, strlen(url));
                } else if (sev->target == png) {
                    prop = send_img(dpy, sev, png, img, bytes);
                } else if (sev->target == tgts) {
                    prop = send_targets(dpy, sev, atom);
                }
                ssev.property = prop;
                ssev.type = SelectionNotify;
                ssev.requestor = sev->requestor;
                ssev.selection = sev->selection;
                ssev.target = sev->target;
                ssev.time = sev->time;

                XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
                break;
        }
    }
}