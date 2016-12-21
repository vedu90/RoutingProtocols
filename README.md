# RoutingProtocols
In this project, i have implemented a simplified version of the Distance Vector Protocol. The
protocol is run on top of servers (behaving as routers) using UDP. Each server runs on a machine
at a pre-defined port number. The servers are able to output their forwarding tables along with
the cost and should be robust to link changes.(Note: we would like you to implement the basic
algorithm: count to infinity not poison reverse. In addition, a server should send out routing packets
only in the following two conditions: a) periodic update and b) the user uses command asking for
one. This is a little different from the original algorithm which immediately sends out update routing
information when routing table changes.

The following commands can be specified at any point during the run of the server:
- update <server-ID1> <server-ID2> <Link Cost> : 
    server-ID1, server-ID2:The link for which the cost is being updated.
    Link Cost: It specifies the new link cost between the source and the destination server. Note that
    this command will be issued to both server-ID1and server-ID2 and involve them to update the
    cost and no other server.
- step : 
    Send routing update to neighbors (triggered/force update)
- packets : 
    Display the number of distance vector packets this server has received since the last invocation of this
    information.
- display : 
    Display the current routing table. And the table should be displayed in a sorted order from small ID to
    big.
- disable<server-ID> : 
    Disable the link to a given server. Doing this “closes” the connection to a given server with
    server-ID.
- crash : 
    Close all connections. This is to simulate server crashes.
