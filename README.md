# Windows Tools

`exprst` - restarts `explorer.exe` process while also preserving and restoring opened Explorer windows. Limitation: currently only works if you set full path in the titles of you Explorer windows! Also too long path might get cliped, and will restoration will fail.

`garbcol` - ultimate silent "garbage collector" for Windows! Just supply it with the path to your Trash Bin folder like this:
```
garbcol C:\\$Recycle.Bin\\S-1-5-21-1780296672-2431193663-1691526161-1001\\$I* 48
```
That `$I*` in the end is important, don't miss it! Also you can supply number of hours after which garbage will get destroyed. It will only delete items that was deleted this hours ago (if you ommit this number the default is 24 hours).

To get more details of how this work refer to my [superuser anwswer](https://superuser.com/a/1736690/1287576).
