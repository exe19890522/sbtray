//******************************************************************************
// main application class
//******************************************************************************

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.applet.*;
import java.net.*;

public class sbstat
{
    //**************************************************************************
    // parameters
    //**************************************************************************
    // application title with version
    final String appTitle   = "sbstat v1.2";
    // application release date
    final String appDate    = "2003-03-08";
    // interval for checking status (secs)
    final int interval      = 30;
    // colors used to indicate server states
    final Color clrUnknown  = Color.lightGray;
    final Color clrDown     = Color.red;
    final Color clrUp       = Color.green;
    // background color (this shows up as the padding between panes and the
    // window border)
    final Color clrBack     = Color.black;
    // size of the main window
    final int mainWndWidth  = 35;
    final int mainWndHeight = 35;
    // padding sizes (between server panes and around edges)
    final int wndPadding    = 2;
    // sound filenames
    final String acUpName   = "upsound.wav";
    final String acDownName = "downsound.wav";
    // sounds enabled?
    boolean enableSounds    = true;

    //**************************************************************************
    // server status panes
    //**************************************************************************
    JPanel paneGame;
    JPanel panePatch;
    JPanel paneLogin;

    //**************************************************************************
    // user interface objects
    //**************************************************************************
    JWindow                mainWnd;
    JFrame                 aboutBox;
    Rectangle              aboutBoxRect;
    JPopupMenu             contextMenu;
    onAboutListener        onAbout;
    onEnableSoundsListener onEnableSounds;
    onQuitListener         onQuit;
    onCloseAboutListener   onCloseAbout;
    JCheckBoxMenuItem      soundMenuItem;

    //**************************************************************************
    // worker thread which periodically checks the server status
    //**************************************************************************
    serverstatus sstat;
    Thread       workerThread;

    //**************************************************************************
    // audio clips and previous game server state
    //**************************************************************************
    AudioClip acUp;
    AudioClip acDown;
    boolean prevGameStatus;

    //**************************************************************************
    // constructor.  initialize and run the application
    //**************************************************************************
    public sbstat() throws Exception
    {
        // set the look and feel to be that of the platform we're running on
        String lafClassName = UIManager.getSystemLookAndFeelClassName();
        UIManager.setLookAndFeel(lafClassName);

        // create server status panes
        paneGame = new JPanel();
        panePatch = new JPanel();
        paneLogin = new JPanel();

        // create top level pane
        JPanel topPane = new JPanel();
        topPane.setLayout(null);
        topPane.setBackground(clrBack);

        // add server status panes to top level pane
        topPane.add(paneGame);
        topPane.add(panePatch);
        topPane.add(paneLogin);

        // create main window and add top level pane
        mainWnd = new JWindow();
        mainWnd.getContentPane().setLayout(null);
        mainWnd.setBackground(clrBack);
        mainWnd.getContentPane().add(topPane);

        // size and position the panes and the main window
        int paneWidth = (int)((double)(mainWndWidth - (wndPadding * 4))/3.0);
        int paneHeight = mainWndHeight - (wndPadding * 2);
        int adjustedMainWndWidth = (paneWidth * 3) + (wndPadding * 4);
        paneGame.setBounds((wndPadding*1)+(paneWidth*0),wndPadding,
                           paneWidth,paneHeight);
        panePatch.setBounds((wndPadding*2)+(paneWidth*1),wndPadding,
                           paneWidth,paneHeight);
        paneLogin.setBounds((wndPadding*3)+(paneWidth*2),wndPadding,
                           paneWidth,paneHeight);
        Dimension screenDim = Toolkit.getDefaultToolkit().getScreenSize();
        int wX = (int)((double)(screenDim.width - adjustedMainWndWidth)/2.0);
        int wY = (int)((double)(screenDim.height - mainWndHeight)/2.0);
        mainWnd.setBounds(wX,wY,adjustedMainWndWidth,mainWndHeight);
        topPane.setBounds(0,0,adjustedMainWndWidth,mainWndHeight);

        // setup the context menu
        contextMenu = new JPopupMenu();
        JMenuItem i;
        i = new JMenuItem("About");
        contextMenu.add(i);
        onAbout = new onAboutListener();
        i.addActionListener(onAbout);
        contextMenu.addSeparator();
        soundMenuItem = new JCheckBoxMenuItem("Enable Sounds",enableSounds);
        contextMenu.add(soundMenuItem);
        onEnableSounds = new onEnableSoundsListener();
        soundMenuItem.addActionListener(onEnableSounds);
        contextMenu.addSeparator();
        i = new JMenuItem("Quit");
        contextMenu.add(i);
        onQuit = new onQuitListener();
        i.addActionListener(onQuit);

        // setup the about box
        aboutBox = new JFrame("About");
        JPanel p = new JPanel();
        p.setBorder(BorderFactory.createEmptyBorder(10,10,10,10));
        p.setLayout(new BoxLayout(p,BoxLayout.Y_AXIS));
        String aboutStrings[] =
        {
            appTitle,
            appDate,
            " ",
            "written by coder_1024",
            " ",
            "Provides Shadowbane server status graphically.",
            "Status is obtained from Sancus' server status page,",
            "not directly from any Ubisoft servers.",
            " ",
            "http://sancus.off.net/serverup.php",
            " ",
            "Visit the sbstat Home Page",
            " ",
            "http://sbstat.sourceforge.net",
            " ",
            "Developed for Sun's Java 2 Platform, Standard Edition",
            "v1.3.1_06",
            " "
        };
        for (int x = 0; x < aboutStrings.length; x++)
        {
            JLabel l = new JLabel(aboutStrings[x]);
            l.setAlignmentX(Component.CENTER_ALIGNMENT);
            if (aboutStrings[x].equalsIgnoreCase(appTitle))
            {
                Font f = l.getFont();
                Font nf = new Font(f.getFontName(),f.getStyle() | Font.BOLD,f.getSize()+2);
                l.setFont(nf);
            }
            p.add(l);
        }
        JButton b = new JButton("Close");
        onCloseAbout = new onCloseAboutListener();
        b.addActionListener(onCloseAbout);
        b.setAlignmentX(Component.CENTER_ALIGNMENT);
        p.add(b);
        aboutBox.getContentPane().add(p);
        aboutBox.pack();
        aboutBoxRect = aboutBox.getBounds();

        // set server status to unkown initially
        displayServerStatusAsUnknown();

        // install mouse handler, used to provide dragging and context menu
        mousehandler mh = new mousehandler(mainWnd,this);
        mainWnd.addMouseListener(mh);
        mainWnd.addMouseMotionListener(mh);

        // load up/down sound clips
        try
        {
            prevGameStatus = false;
            acUp = null;
            acDown = null;
            URL u;
            u = getClass().getResource(acUpName);
            acUp = Applet.newAudioClip(u);
            u = getClass().getResource(acDownName);
            acDown = Applet.newAudioClip(u);
        }
        catch (Exception e)
        {
            acUp = null;
            acDown = null;
        }

        // show the main window and request focus
        mainWnd.setVisible(true);
        mainWnd.requestFocus();

        // kick off the worker thread which periodically downloads status
        sstat = new serverstatus(this,interval);
        workerThread = new Thread(sstat);
        workerThread.run();
    }

    //**************************************************************************
    // update server display to indicate server status
    //**************************************************************************
    public void displayServerStatus(boolean gameStatus,
                                    boolean patchStatus,
                                    boolean loginStatus)
    {
        boolean playSound = false;
        if (gameStatus != prevGameStatus)
        {
            prevGameStatus = gameStatus;

            if (enableSounds)
            {
                playSound = true;
            }
        }

        // game server
        if (gameStatus)
        {
            paneGame.setBackground(clrUp);

            if (playSound && (acUp != null))
            {
                acUp.play();
            }
        }
        else
        {
            paneGame.setBackground(clrDown);

            if (playSound && (acDown != null))
            {
                acDown.play();
            }
        }

        // server
        if (patchStatus)
        {
            panePatch.setBackground(clrUp);
        }
        else
        {
            panePatch.setBackground(clrDown);
        }

        // server
        if (loginStatus)
        {
            paneLogin.setBackground(clrUp);
        }
        else
        {
            paneLogin.setBackground(clrDown);
        }
    }

    //**************************************************************************
    // update server display to indicate server status is unknown
    //**************************************************************************
    public void displayServerStatusAsUnknown()
    {
        paneGame.setBackground(clrUnknown);
        panePatch.setBackground(clrUnknown);
        paneLogin.setBackground(clrUnknown);
    }

    //**************************************************************************
    // display the context menu
    //**************************************************************************
    public void displayContextMenu(Component comp, int x, int y)
    {
        contextMenu.show(comp,x,y);
    }


    //**************************************************************************
    // About menu item was selected
    //**************************************************************************
    class onAboutListener implements ActionListener
    {
        public void actionPerformed(ActionEvent e)
        {
            // display the about box
            Dimension screenDim = Toolkit.getDefaultToolkit().getScreenSize();
            int x = (int)((double)(screenDim.width - aboutBoxRect.width)/2.0);
            int y = (int)((double)(screenDim.height - aboutBoxRect.height)/2.0);
            aboutBox.setBounds(x,y,aboutBoxRect.width,aboutBoxRect.height);

            aboutBox.setVisible(true);
            aboutBox.requestFocus();
        }
    }

    //**************************************************************************
    // Enable Sounds was selected
    //**************************************************************************
    class onEnableSoundsListener implements ActionListener
    {
        public void actionPerformed(ActionEvent e)
        {
            enableSounds = !enableSounds;
            soundMenuItem.setState(enableSounds);
        }
    }

    //**************************************************************************
    // Quit menu item was selected
    //**************************************************************************
    class onQuitListener implements ActionListener
    {
        public void actionPerformed(ActionEvent e)
        {
            // exit the application
            System.exit(0);
        }
    }

    //**************************************************************************
    // about box close button was pressed
    //**************************************************************************
    class onCloseAboutListener implements ActionListener
    {
        public void actionPerformed(ActionEvent e)
        {
            aboutBox.setVisible(false);
        }
    }

    //**************************************************************************
    // main entry point
    //**************************************************************************
    public static void main(String[] args)
    {
        try
        {
            // create the application object.  this initializes the application
            // and starts it running
            sbstat app = new sbstat();
        }

        // just in case
        catch (Exception e)
        {
            System.err.println("Exception: " + e);
            e.printStackTrace();
        }
    }
}
