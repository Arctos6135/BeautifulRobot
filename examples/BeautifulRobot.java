/*
 * This example uses the SerialPort class from WPILib to interface the BeautifulRobot. 
 */

package frc.robot.components;

import edu.wpi.first.wpilibj.SerialPort;

/**
 * <p>
 * The BeautifulRobot&#8482; is an 8051-based WS2812B RGB LED strip controller, interfaced with UART.
 * </p>
 * For more information, refer to the README in its repository on <a href="https://github.com/Arctos6135/BeautifulRobot#beautifulrobot">GitHub</a>.
 */
public class BeautifulRobot {
    
    protected SerialPort serial;

    /**
     * <p>
     * All the valid operations for the BeautifulRobot&#8482; LED controller.
     * </p>
     * For information about what all the operations do, as well as their valid range of parameters, refer to
     * <a href="https://github.com/Arctos6135/BeautifulRobot#valid-operations-and-parameters">this table</a>.
     */
    enum Operation {

        ENABLE(0x01), BRIGHTNESS(0x02), MODE(0x03), COLOR(0x04), DIRECTION(0x05), LED_COUNT(0x06),
        SPEED_HIGH(0x07), SPEED_LOW(0x08);

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
        //Should default to 8 data bits, one stop bit, and no parity bit
        serial = new SerialPort(9600, port);

        serial.disableTermination();
        //Write immediately
        serial.setWriteBufferMode(SerialPort.WriteBufferMode.kFlushOnAccess);
        //Set read buffer size to 2 since the BeautifulRobot echos back the operation and the parameter
        serial.setReadBufferSize(2);
    }

    /**
     * Writes a command to the BeautifulRobot&#8482;. This method takes slightly more than 150ms to execute.
     * There is no error checking to make sure the right values were received. Use {@link #writeCommandWithCheck(Operation, byte, int)}
     * instead.
     * @param op The operation
     * @param param The parameter
     */
    public void writeCommand(Operation op, byte param) {
        //Convert all the bytes to arrays
        byte[] opArr = new byte[] { op.getCode() };
        byte[] paramArr = new byte[] { param };
        byte[] sync = new byte[] { (byte) 0xFF };

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

    /**
     * <p>
     * Writes a command to the BeautifulRobot&#8482;. Each write operation takes slightly more than 150ms.
     * This method performs error checking to make sure the right values were received by the BeautifulRobot&#8482;.
     * </p>
     * <p>
     * If the BeautifulRobot&#8482; takes longer than 100ms to respond, this method times out and returns {@code false}.
     * If the value received by the BeautifulRobot&#8482; was not correct, this method will try again, until a total of
     * 2 attempts has been reached.
     * </p>
     * @param op The operation
     * @param param The parameter
     * @return {@code true} if write successful, {@code false} if timed out or maximum number of attempts reached.
     */
    public boolean writeCommandWithCheck(Operation op, byte param) {
        return writeCommandWithCheck(op, param, 2);
    }
    /**
     * <p>
     * Writes a command to the BeautifulRobot&#8482;. Each write operation takes slightly more than 150ms.
     * This method performs error checking to make sure the right values were received by the BeautifulRobot&#8482;.
     * </p>
     * <p>
     * If the BeautifulRobot&#8482; takes longer than 100ms to respond, this method times out and returns {@code false}.
     * If the value received by the BeautifulRobot&#8482; was not correct, this method will try again, until the specified
     * number of attempts has been reached.
     * </p>
     * @param op The operation
     * @param param The parameter
     * @param maxAttempts The maximum number of attempts to make before giving up
     * @return {@code true} if write successful, {@code false} if timed out or maximum number of attempts reached.
     */
    public boolean writeCommandWithCheck(Operation op, byte param, int maxAttempts) {
        //Keep track of the starting time
        long start;

        while(true) {
            //Clear the read buffer just in case
            serial.read(serial.getBytesReceived());

            //Send the data
            writeCommand(op, param);

            start = System.currentTimeMillis();
            //Wait for echo
            while(serial.getBytesReceived() < 2) {
                //Time out after 100ms
                if(System.currentTimeMillis() - start >= 100) {
                    return false;
                }
            }
            byte[] echo = serial.read(2);
            
            //Verify that the echo is the same as the bytes we sent
            if(echo[0] == op.getCode() && echo[1] == param) {
                return true;
            }
            //Otherwise check if we exceeded the max number of attempts
            --maxAttempts;
            if(maxAttempts <= 0) {
                return false;
            }
        }
    }
}
