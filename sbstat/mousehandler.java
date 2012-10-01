//******************************************************************************
// handles mouse interaction with the user, providing draggability for the
// main window and access to the context menu
//******************************************************************************

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import java.util.*;

class mousehandler extends MouseInputAdapter
{
    // main window and application object
    JWindow window;
    sbstat app;

    // point used when dragging the window
    Point origin = new Point();

    //**************************************************************************
    // constructor.  store the window and application objects for later use
    //**************************************************************************
    public mousehandler(JWindow window, sbstat app)
    {
        this.window = window;
        this.app = app;
    }

    //**************************************************************************
    // mouse was pressed
    //**************************************************************************
    public void mousePressed(MouseEvent e)
    {
        // Remember offset into window for dragging
        origin.x = e.getX();
        origin.y = e.getY();

        // display popup?
        if (e.isPopupTrigger())
        {
            app.displayContextMenu(window,e.getX(),e.getY());
        }
    }

    //**************************************************************************
    // mouse was released
    //**************************************************************************
    public void mouseReleased(MouseEvent e)
    {
        // display popup?
        if (e.isPopupTrigger())
        {
            app.displayContextMenu(window,e.getX(),e.getY());
        }
    }

    //**************************************************************************
    // mouse is being dragged
    //**************************************************************************
    public void mouseDragged(MouseEvent e)
    {
        // move window relative to drag start
        Point p = window.getLocation();
        window.setLocation(p.x + e.getX() - origin.x,p.y + e.getY() - origin.y);
    }
}
