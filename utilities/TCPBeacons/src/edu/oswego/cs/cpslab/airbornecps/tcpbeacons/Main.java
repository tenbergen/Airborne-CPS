package edu.oswego.cs.cpslab.airbornecps.tcpbeacons;

/**
 * A tool to test Airborne-CPS TCAS plugin connections via TCP. Sends
 * @author Bastian Tenbergen (bastian.tenbergen@oswego.edu)
 * @since 2022-03-14
 * @version 2022-03-14
 */
public class Main {

    //help text
    private final static String helptext = "Syntax: java -jar tcpbeacons.jar (-server PORT? FILENAME | -client IP PORT FILENAME).";
    private final static int PORT = 1901; //default port

    /**
     * Runs the tool in server or client mode.
     * @param args Syntax: java -jar tcpbeacons.jar (-server PORT? FILENAME | -client IP PORT FILENAME).
     */
    public static void main(String[] args) {
        if (args.length == 0) System.out.println(helptext);
        else {
            switch (args[0]) {
                case "-server": {
                    if (args.length == 2) {
                        new Server(PORT).serve(args[1]);
                    } else if (args.length == 3) {
                        new Server(Integer.parseInt(args[1])).serve(args[2]);
                    } else {
                        System.out.println("Syntax error.\n   " + helptext);
                    }
                    break;
                }
                case "-client": {
                    if (args.length == 4) {
                        new Client(args[1], Integer.parseInt(args[2])).connect(args[3]);
                    } else {
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