//******************************************************************************
// used to update the main UI in a thread-safe manner
//******************************************************************************

public class serverstatusupdater implements Runnable
{
    sbstat app;
    boolean gameStatus, patchStatus, loginStatus;
    boolean unknownStatus;

    public serverstatusupdater(sbstat  app,
                               boolean gameStatus,
                               boolean patchStatus,
                               boolean loginStatus)
    {
        this.app = app;
        this.gameStatus = gameStatus;
        this.patchStatus = patchStatus;
        this.loginStatus = loginStatus;
        unknownStatus = false;
    }

    public serverstatusupdater(sbstat app)
    {
        this.app = app;
        this.gameStatus = false;
        this.patchStatus = false;
        this.loginStatus = false;
        this.unknownStatus = true;
    }

    public void run()
    {
        if (unknownStatus)
        {
            app.displayServerStatusAsUnknown();
        }
        else
        {
            app.displayServerStatus(gameStatus,patchStatus,loginStatus);
        }
    }
}
