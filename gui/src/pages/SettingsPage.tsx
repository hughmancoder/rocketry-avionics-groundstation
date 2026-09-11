import { useState, useEffect, useCallback, useRef } from "react";
import { Button } from "@/components/ui/button";
import {
  Select,
  SelectTrigger,
  SelectValue,
  SelectContent,
  SelectGroup,
  SelectLabel,
  SelectItem,
} from "@/components/ui/select";
import { STATUS, Telemetry } from "@/types";
// import { startMockTelemetry } from "@/mock";

type SettingsPageProps = {
  portStatus: STATUS;
  setPortStatus: React.Dispatch<React.SetStateAction<STATUS>>;
  telemetryData: Telemetry[];
  setTelemetryData: React.Dispatch<React.SetStateAction<Telemetry[]>>;
};

export default function SettingsPage({
  portStatus,
  setPortStatus,
  telemetryData,
  setTelemetryData,
}: SettingsPageProps) {
  const [ports, setPorts] = useState<SerialPort[]>([]);
  const [selectedPort, setSelectedPort] = useState<SerialPort | null>(null);
  const [rawData, setRawData] = useState<string>("");
  const readerRef = useRef<ReadableStreamDefaultReader | null>(null); 

  // MOCK data
  /*
    useEffect(() => {
      const stopMock = startMockTelemetry((packet) => {
        setTelemetryData((prev) => [...prev, packet]);
      });
  
      return () => stopMock(); // Cleanup on unmount
    }, []);
  */

  // Check if "connected" or not
  const isConnected = portStatus === STATUS.CONNECTED;
  const loadPorts = useCallback(async () => {
    try {
      console.log("INFO: Requesting serial ports");
      
      // Request the user to select a port
      const port = await navigator.serial.requestPort();
  
      if (port) {
        setPorts([port]); // Store in state
        setPortStatus(STATUS.AWAITING);
        console.log("Selected port:", port);
      }
    } catch (err) {
      console.error("Error fetching ports:", err);
    }
  }, [setPortStatus]);
  

  const onSelectPort = (portId: string) => {
    const portObj = ports.find(
      (p) => String(p.getInfo().usbProductId) === portId
    );
    setSelectedPort(portObj || null);
  };

  // Opens the selected port, reads CSV lines, and updates telemetry data
  const connectPort = useCallback(async () => {
    if (!selectedPort) return;
    try {
        console.log("INFO: Connecting to port:", selectedPort);
      await selectedPort.open({ baudRate: 115200 });
      setPortStatus(STATUS.CONNECTED);

      const reader = selectedPort.readable?.getReader();
      if (!reader) {
        console.log("ERROR: No reader available");
        return;
      }
      readerRef.current = reader; // Store reader in ref

      let buffer = "";
      while (true) {
        const { value, done } = await reader.read();
        if (done || !value) break;

        // Convert incoming bytes to string
        buffer += new TextDecoder().decode(value);

        // Split on newlines; keep last partial line in buffer
        const lines = buffer.split("\n");
        buffer = lines.pop() || "";

        setRawData((prev) => prev + buffer);

        // Parse each complete line (comma-separated fields)
        for (const line of lines) {
          const parts = line.trim().split(",");
          if (parts.length < 12) continue; // skip incomplete

          const packet: Telemetry = {
            time: Number(parts[0]),
            Temp: Number(parts[1]),
            pressure: Number(parts[2]),
            altitude: Number(parts[3]),
            accX: Number(parts[4]),
            accY: Number(parts[5]),
            accZ: Number(parts[6]),
            angVelX: Number(parts[7]),
            angVelY: Number(parts[8]),
            angVelZ: Number(parts[9]),
            lat: Number(parts[10]),
            lon: Number(parts[11])
          };
          // Push the new packet into parent state
          setTelemetryData((prev) => [...prev, packet]);
        }
      }
    } catch (err) {
      console.error("Failed to connect:", err);
      setPortStatus(STATUS.DISCONNECTED);
    }
  }, [selectedPort, setPortStatus, setTelemetryData]);

  // Closes the port
  const disconnectPort = useCallback(async () => {
    if (!selectedPort) return;
    try {
      if (readerRef.current) {
        await readerRef.current.cancel();
        readerRef.current.releaseLock();
        readerRef.current = null;
      }
      await selectedPort.close();
      setPortStatus(STATUS.DISCONNECTED);
    } catch (err) {
      console.error("Error closing port:", err);
      setPortStatus(STATUS.DISCONNECTED);
    }
  }, [selectedPort, setPortStatus]);

  useEffect(() => {
    if (selectedPort) {
      setPortStatus(STATUS.AWAITING);
    }
  }, [selectedPort, setPortStatus]);

  return (
    <div className="pt-8 px-4 sm:px-8 md:px-16">
      <Select
        value={selectedPort ? String(selectedPort.getInfo().usbProductId) : ""}
        onValueChange={onSelectPort}
      >
        <SelectTrigger className="w-full text-black">
          <SelectValue placeholder="Select a serial port" />
        </SelectTrigger>
        <SelectContent>
          <SelectGroup>
            <SelectLabel>Serial Ports</SelectLabel>
            {ports.map((port, i) => {setPortStatus(STATUS.DISCONNECTED);
              const info = port.getInfo();
              const portId = String(info.usbProductId);
              return (
                <SelectItem key={i} value={portId}>
                  USB PID: {portId} (VID: {info.usbVendorId})
                </SelectItem>
              );
            })}
          </SelectGroup>
        </SelectContent>
      </Select>

      <div className="flex flex-col sm:flex-row space-y-4 sm:space-y-0 sm:space-x-4 mt-4">
        <Button
          onClick={loadPorts}
          className="bg-gray-700 hover:bg-gray-800 w-full sm:w-auto"
        >
          Load Serial Ports
        </Button>

        <Button
          onClick={disconnectPort}
          className="bg-gray-700 hover:bg-gray-800 w-full sm:w-auto"
        >
          Clear ports
        </Button>
        <Button
          // onClick={isConnected ? disconnectPort : startMockTelemetry} // NOTE: for mock data
          onClick={isConnected ? disconnectPort : connectPort} 
          disabled={!selectedPort}
          className={`text-white w-full sm:w-auto ${
            selectedPort
              ? "bg-yellow-500 hover:bg-yellow-600"
              : "bg-gray-400 cursor-not-allowed"
          }`}
        >
          {isConnected ? "Disconnect" : "Connect"}
        </Button>
      </div>

      <div className="mt-16">
      <p>Port Status: {portStatus}</p>


        <p>Raw Serial Data:</p>
        <pre className="mt-2 p-2 bg-gray-100 text-sm overflow-auto h-40 text-blue-900">
          {rawData}
        </pre>

        <p>Telemetry Data</p>
        <pre className="mt-2 p-2 bg-gray-100 text-sm overflow-auto h-80 text-blue-900">
          {JSON.stringify(telemetryData, null, 2)}
        </pre>
      </div>
    </div>
  );
}
