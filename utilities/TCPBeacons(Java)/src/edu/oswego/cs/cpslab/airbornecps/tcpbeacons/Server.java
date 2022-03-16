package edu.oswego.cs.cpslab.airbornecps.tcpbeacons;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.Scanner;
import java.net.ServerSocket;

/**
 * A simple TCP server that receives and sends Airborne-CPS beacons.
 * @author Bastian Tenbergen (bastian.tenbergen@oswego.edu)
 * @since 2022-03-12
 * @version 2022-03-12
 */
public class Server {

    //the port to serve on.
    private int port;

    /**
     * Creates a new server instance on default port 1901.
     */
    public Server() {
        this(1901);
    }

    /**
     * Creates a new server instance on a specified port.
     * @param port the port to serve on
     */
    public Server(int port) {
        this.port = port;
    }

    /**
     * Begins accepting clients. For each client, a reader and a writer thread is created. Client beacons are printed to
     * System.out with client address as prefix. Beacons from file are sent to each connecting client, one row at a time.
     * @param file File containing beacons. Each row must conform to the format "nMACnIPnLATnLONGnALTmeters", 'n' separates mac address, ip address, lattitude, longitute, and altitude in meters.
     */
    public void serve(String file) {
        try (ServerSocket serverSocket = new ServerSocket(port)) {
            System.out.println("Server is listening on port " + port);
            while (true) {
                Socket socket = serverSocket.accept();
                System.out.println("New client connected: " + socket.getInetAddress().getHostAddress());
                //reader threat for this client
                new Thread(() -> {
                    Scanner reader = null;
                    try {
                        reader = new Scanner(socket.getInputStream());
                        while (reader.hasNext()) {
                            System.out.println(socket.getInetAddress().getHostAddress() + ": " + reader.nextLine());
                        }
                        System.out.println("Client " + socket.getInetAddress().getHostAddress() + " disconnected.");
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }).start();
                //writer thread for this client
                new Thread(() -> {
                    PrintWriter writer = null;
                    Scanner scanner = null;
                    try {
                        writer = new PrintWriter(socket.getOutputStream(), true);
                        while (true) {
                            scanner = new Scanner(new File(file));
                            while (scanner.hasNextLine()) {
                                writer.println(scanner.nextLine());
                                Thread.sleep(150);
                            }
                        }
                    } catch (IOException | InterruptedException e) {
                        System.out.println("Exception while writing to client: " + e.getMessage());
                        e.printStackTrace();
                    }
                }).start();
            }
        } catch (IOException e) {
            System.out.println("Server exception: " + e.getMessage());
            e.printStackTrace();
        }
    }
}