/*
 * This example uses the SerialPort class from WPILib to interface the BeautifulRobot. 
 */

package frc.robot.components;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import edu.wpi.first.wpilibj.SerialPort;

/**
 * <p>
 * The BeautifulRobot&#8482; is an 8051-based WS2812B RGB LED strip controller, interfaced with UART.
 * This class provides methods to write commands to the BeautifulRobot&#8482;, write commands with error checking,
 * and write commands concurrently (since each write takes at least 150ms).
 * </p>
 * For more information, refer to the README in its repository on <a href="https://github.com/Arctos6135/BeautifulRobot#beautifulrobot">GitHub</a>.
 */
public class BeautifulRobot {
    
    protected final SerialPort serial;
    //A single-threaded ExecutorService is used for concurrent writes
    protected final ExecutorService writeExecutor;

    /**
     * <p>
     * All the valid operations for the BeautifulRobot&#8482; LED controller.
     * </p>
     * For information about what all the operations do, as well as their valid range of parameters, refer to
     * <a href="https://github.com/Arctos6135/BeautifulRobot#valid-operations-and-parameters">this table</a>.
     */
    enum Operation {

        ENABLE(0x01), BRIGHTNESS(0x02), MODE(0x03), COLOR(0x04), DIRECTION(0x05), LED_COUNT(0x06),
        SPEED_HIGH(0x07), SPEED_LOW(0x08), RESET(0x09);

        private byte code;
        Operation(int code) {
            this((byte) code);
        }
        Operation(byte code) {
            this.code = code;
        }
        byte getCode() {
            return this.code;
        }
    }
    
    /**
     * Creates a new {@code BeautifulRobot} object representing a BeautifulRobot&#8482; connected to a serial port.
     * @param port The serial port it's connected to
     */
    public BeautifulRobot(SerialPort.Port port) {
        writeExecutor = Executors.newSingleThreadExecutor();
        //Should default to 8 data bits, one stop bit, and no parity bit
        serial = new SerialPort(9600, port);

        serial.disableTermination();
        //Write immediately
        serial.setWriteBufferMode(SerialPort.WriteBufferMode.kFlushOnAccess);
        //Set read buffer size to 2 since the BeautifulRobot echos back the operation and the parameter
        serial.setReadBufferSize(2);
    }

    /**
     * Writes a command to the BeautifulRobot&#8482;.
     * There is no error checking to make sure the right values were received. Use {@link #writeCommandWithCheck(Operation, byte, int)}
     * instead.
     * <p>
     * This method will block for at least 150ms to execute. Consider using {@link #concurrentWrite(Operation, byte)} instead.
     * </p>
     * @param op The operation
     * @param param The parameter
     */
    public void writeCommand(Operation op, byte param) {
        //Convert all the bytes to arrays
        byte[] opArr = new byte[] { op.getCode() };
        byte[] paramArr = new byte[] { param };
        byte[] sync = new byte[] { (byte) 0xFF };
        
        synchronized(this) {
            try {
                //Write each one and delay
                serial.write(opArr, 1);
                Thread.sleep(50);
                serial.write(paramArr, 1);
                Thread.sleep(50);
                serial.write(sync, 1);
                Thread.sleep(50);
            }
            catch(InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * <p>
     * Writes a command to the BeautifulRobot&#8482;.
     * This method performs error checking to make sure the right values were received by the BeautifulRobot&#8482;.
     * </p>
     * <p>
     * If the value received by the BeautifulRobot&#8482; was not correct, or if the BeautifulRobot&#8482; fails to
     * respond within 100ms, this method will try again, until a total of 2 attempts have failed.
     * </p>
     * <p>
     * This method will block for at least 150ms to execute, and maybe even more depending on the number of failed
     * attempts. Consider using {@link #concurrentWriteWithCheck(Operation, byte)} instead.
     * </p>
     * @param op The operation
     * @param param The parameter
     * @return {@code true} if write successful, {@code false} if the maximum number of attempts was reached.
     */
    public synchronized boolean writeCommandWithCheck(Operation op, byte param) {
        return writeCommandWithCheck(op, param, 2);
    }
    /**
     * <p>
     * Writes a command to the BeautifulRobot&#8482;. Each write operation takes slightly more than 150ms.
     * This method performs error checking to make sure the right values were received by the BeautifulRobot&#8482;.
     * </p>
     * <p>
     * If the value received by the BeautifulRobot&#8482; was not correct, or if the BeautifulRobot&#8482; fails to
     * respond within 100ms, this method will try again, until the specified number of attempts has been reached.
     * </p>
     * <p>
     * This method will block for at least 150ms to execute, and maybe even more depending on the number of failed
     * attempts. Consider using {@link #concurrentWriteWithCheck(Operation, byte, int)} instead.
     * </p>
     * @param op The operation
     * @param param The parameter
     * @param maxAttempts The maximum number of attempts to make before giving up
     * @return {@code true} if write successful, {@code false} if the maximum number of attempts was reached.
     */
    public synchronized boolean writeCommandWithCheck(Operation op, byte param, int maxAttempts) {
        //Keep track of the starting time
        long start;

        while(true) {
            //Clear the read buffer just in case
            serial.read(serial.getBytesReceived());

            //Send the data
            writeCommand(op, param);

            start = System.currentTimeMillis();
            //Wait for echo
            boolean echoReceived = true;
            while(serial.getBytesReceived() < 2) {
                //Time out after 100ms
                if(System.currentTimeMillis() - start >= 100) {
                    echoReceived = false;
                    break;
                }
            }
            //Verify echo if received
            if(echoReceived) {
                byte[] echo = serial.read(2);
                
                //Verify that the echo is the same as the bytes we sent
                if(echo[0] == op.getCode() && echo[1] == param) {
                    return true;
                }
            }
            //Otherwise check if we exceeded the max number of attempts
            --maxAttempts;
            if(maxAttempts <= 0) {
                return false;
            }
        }
    }

    /**
     * Concurrently writes a command to the BeautifulRobot&#8482;. The write takes at least 150ms, but this method does not block.
     * There is no error checking to make sure the right values were received. Use {@link #concurrentWriteWithCheck(Operation, byte, int)}
     * instead.
     * @param op The operation
     * @param param The parameter
     * @return A {@code Future} representing pending completion of the write
     */
    public Future<?> concurrentWrite(Operation op, byte param) {
        return writeExecutor.submit(() -> {
            writeCommand(op, param);
        });
    }
    /**
     * <p>
     * Concurrently writes a command to the BeautifulRobot&#8482;. The write takes at least 150ms, but this method does not block.
     * This method performs error checking to make sure the right values were received by the BeautifulRobot&#8482;.
     * </p>
     * <p>
     * If the value received by the BeautifulRobot&#8482; was not correct, or if the BeautifulRobot&#8482; fails to
     * respond within 100ms, this method will try again, until a total of 2 attempts have failed.
     * </p>
     * @param op The operation
     * @param param The parameter
     * @return A {@code Future} representing pending completion of the write. The result is a true if the write was successful,
     * false otherwise.
     */
    public Future<Boolean> concurrentWriteWithCheck(Operation op, byte param) {
        return writeExecutor.submit(() -> {
            return writeCommandWithCheck(op, param);
        });
    }
    /**
     * <p>
     * Writes a command to the BeautifulRobot&#8482;. The write takes at least 150ms, but this method does not block.
     * This method performs error checking to make sure the right values were received by the BeautifulRobot&#8482;.
     * </p>
     * <p>
     * If the value received by the BeautifulRobot&#8482; was not correct, or if the BeautifulRobot&#8482; fails to
     * respond within 100ms, this method will try again, until the specified number of attempts has been reached.
     * </p>
     * @param op The operation
     * @param param The parameter
     * @param maxAttempts The maximum number of attempts to make before giving up
     * @return A {@code Future} representing pending completion of the write. The result is a true if the write was successful,
     * false otherwise.
     */
    public Future<Boolean> concurrentWriteWithCheck(Operation op, byte param, int maxAttempts) {
        return writeExecutor.submit(() -> {
            return writeCommandWithCheck(op, param, maxAttempts);
        });
    }
}
