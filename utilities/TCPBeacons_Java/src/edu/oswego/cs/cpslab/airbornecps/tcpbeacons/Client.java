package edu.oswego.cs.cpslab.airbornecps.tcpbeacons;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.Scanner;

/**
 * A simple TCP client that receives and sends Airborne-CPS beacons.
 * @author Bastian Tenbergen (bastian.tenbergen@oswego.edu)
 * @since 2022-03-12
 * @version 2022-03-12
 */
public class Client {

    private String ip; //the IP to connect to
    private int port;  //the port the server is running on
    private int delay; //the value by which to delay message relay

    /**
     * Creates a new client instance for connecting to localhost on default port 1901.
     */
    public Client() {
        this("127.0.0.1", 1901, Main.Mode.normal);
    }

    /**
     * Creates a new client instance on a specified port.
     * @param ip the server address
     * @param port the port to connect to
     */
    public Client(String ip, int port, Main.Mode mode) {
        this.ip = ip;
        this.port = port;
        switch (mode) {
            case normal: this.delay = 0; break;
            case slower: this.delay = 100; break;
            case slowest: this.delay = 500; break;
        }
    }

    /**
     * Connects to server. Starts a separate thread for writing own beacons from 'file', one row at a time.
     * Begins permanently reading beacons from server, which are printed to System.out.
     * @param file File containing beacons. Each row must conform to the format "nMACnIPnLATnLONGnALTmeters", 'n' separates mac address, ip address, lattitude, longitute, and altitude in meters.
     */
    public void connect(String file) {
        try (Socket socket = new Socket(this.ip, this.port)) {
            System.out.println("Connected to " + socket.getInetAddress().getHostName() + " at " + socket.getInetAddress().getHostAddress() + " on port " + socket.getPort());
            Scanner reader = new Scanner(socket.getInputStream());
            PrintWriter writer = new PrintWriter(socket.getOutputStream(), true);
            //writer thread
            new Thread(() -> {
                Scanner scanner = null;
                try {
                    while (true) {
                        scanner = new Scanner(new File(file));
                        while (scanner.hasNextLine()) {
                            writer.println(scanner.nextLine());
                            Thread.sleep(this.delay);
                        }
                        Thread.sleep(10);
                    }
                } catch (IOException | InterruptedException e) {
                    System.out.println("Exception while writing to server: " + e.getMessage());
                    e.printStackTrace();
                }
            }).start();
            //read using this thread
            while (reader.hasNext()) {
                System.out.println(reader.nextLine());
            }
            System.out.println("Server closed the connection.");
        } catch (IOException e) {
            System.out.println("Client exception: " + e.getMessage());
            e.printStackTrace();
        }
    }
}