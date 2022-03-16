package edu.oswego.cs.cpslab.airbornecps.tcpbeacons;

/**
 * A tool to test Airborne-CPS TCAS plugin connections via TCP. Sends
 * @author Bastian Tenbergen (bastian.tenbergen@oswego.edu)
 * @since 2022-03-14
 * @version 2022-03-14
 */
public class Main {

    //toggle update rate for client and server
    protected enum Mode { normal, slower, slowest }

    //help text
    private final static String helptext = "Syntax: java -jar tcpbeacons.jar (-server PORT? FILENAME | -client IP PORT FILENAME) slow? slow?";

    /**
     * Runs the tool in server or client mode.
     * @param args Syntax: java -jar tcpbeacons.jar (-server PORT? FILENAME | -client IP PORT FILENAME) slow? slow?
     */
    public static void main(String[] args) {
        if (args.length == 0) System.out.println(helptext);
        else {
            switch (args[0]) {
                case "-server": {
                    if (args.length == 2) {
                        new Server().serve(args[1]);
                    } else if (args.length == 3) {
                        new Server(Integer.parseInt(args[1]), Mode.normal).serve(args[2]);
                    } else if (args.length == 4 && args[3].equals("slow")) {
                        new Server(Integer.parseInt(args[1]), Mode.slower).serve(args[2]);
                    } else if (args.length == 5  && args[3].equals("slow") && args[4].equals("slow")) {
                        new Server(Integer.parseInt(args[1]), Mode.slowest).serve(args[2]);
                    }
                    else {
                        System.out.println("Syntax error.\n   " + helptext);
                    }
                    break;
                }
                case "-client": {
                    if (args.length == 4) {
                        new Client(args[1], Integer.parseInt(args[2]), Mode.normal).connect(args[3]);
                    } else if (args.length == 5 && args[4].equals("slow")) {
                        new Client(args[1], Integer.parseInt(args[2]), Mode.slower).connect(args[3]);
                    } else if (args.length == 6 && args[4].equals("slow") && args[5].equals("slow")) {
                        new Client(args[1], Integer.parseInt(args[2]), Mode.slowest).connect(args[3]);
                    }  else {
                        System.out.println("Syntax error.\n   " + helptext);
                    }
                    break;
                }
                default:
                    System.out.println("Syntax error.\n   " + helptext);
                    break;
            }
        }
    }
}