//******************************************************************************
// downloads server status periodically
//******************************************************************************

import javax.swing.*;
import java.net.*;
import java.io.*;

class serverstatus implements Runnable
{
    sbstat app;
    int interval;

    boolean gameStatus = false,
            patchStatus = false,
            loginStatus = false;

    //**************************************************************************
    // parameters
    //**************************************************************************
    final String host  = "sancus.off.net";
    final int port     = 80;
    final String file  = "/serverup.php";
    final int numLines = 12;

    //**************************************************************************
    // constructor.  provide the application object and interval for status
    // checking
    //**************************************************************************
    public serverstatus(sbstat app, int interval)
    {
        // save off for later
        this.app = app;
        this.interval = interval;
    }

    //**************************************************************************
    // check server status
    //**************************************************************************
    void getServerStatus() throws Exception
    {
        // establish a connection with the server
        Socket sock = new Socket(host,port);

        try
        {
            // setup reader/writer
            PrintWriter out = new PrintWriter(sock.getOutputStream(),true);
            BufferedReader in =
                new BufferedReader(new InputStreamReader(sock.getInputStream()));

            // send request
            String request = "GET " + file + " HTTP/1.0\n" +
                             "Host: " + host + "\n";
            out.println(request);

            // read response
            String resp = "";
            for (int x = 0; x < numLines; x++)
            {
                resp = resp + in.readLine();
            }

            // check for server status
            gameStatus = false;
            patchStatus = false;
            loginStatus = false;
            if (resp.indexOf("G:U") >= 0)
            {
                gameStatus = true;
            }
            if (resp.indexOf("P:U") >= 0)
            {
                patchStatus = true;
            }
            if (resp.indexOf("L:U") >= 0)
            {
                loginStatus = true;
            }

            // close reader/writer
            out.close();
            in.close();

            // close connection with the server
            sock.close();
        }

        // ensure the socket gets closed
        catch (Exception e)
        {
            sock.close();
            throw e;
        }
    }

    //**************************************************************************
    // main entry point
    //**************************************************************************
    public void run()
    {
        boolean firstIter = true;

        // loop forever...
        while(true)
        {
            try
            {
                // pause between checks for the specified interval
                if (!firstIter)
                {
                    Thread.sleep(interval*1000);
                }
                firstIter = false;

                // get the latest server status
                getServerStatus();

                // notify the application
                serverstatusupdater updater =
                    new serverstatusupdater(app,
                                            gameStatus,
                                            patchStatus,
                                            loginStatus);
                SwingUtilities.invokeLater(updater);
            }

            // if an error ocurred while trying to obtain the server status
            // notify the application and output the error details
            catch(Exception e)
            {
                serverstatusupdater updater =
                    new serverstatusupdater(app);
                SwingUtilities.invokeLater(updater);

                System.err.println("Exception: " + e);
                e.printStackTrace();
            }
        }
    }
}
