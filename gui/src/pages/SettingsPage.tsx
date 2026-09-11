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
import {
  DataSource,
  parseCotsGpsTelemetry,
  parseSradTelemetry,
} from "@/serialParsers";
// import { startMockTelemetry } from "@/mock";

type SettingsPageProps = {
  portStatus: STATUS;
  setPortStatus: React.Dispatch<React.SetStateAction<STATUS>>;
  telemetryData: Telemetry[];
  setTelemetryData: React.Dispatch<React.SetStateAction<Telemetry[]>>;
  launchSite: [number, number];
  setLaunchSite: React.Dispatch<React.SetStateAction<[number, number]>>;
};

export default function SettingsPage({
  portStatus,
  setPortStatus,
  telemetryData,
  setTelemetryData,
  launchSite,
  setLaunchSite,
}: SettingsPageProps) {
  const [ports, setPorts] = useState<SerialPort[]>([]);
  const [selectedPort, setSelectedPort] = useState<SerialPort | null>(null);
  const [rawData, setRawData] = useState<string>("");
  const [dataSource, setDataSource] = useState<DataSource>("srad");
  const [transport, setTransport] = useState<"serial" | "websocket">("serial");
  const [websocketUrl, setWebsocketUrl] = useState("ws://localhost:8765");
  const [launchLongitude, setLaunchLongitude] = useState(String(launchSite[0]));
  const [launchLatitude, setLaunchLatitude] = useState(String(launchSite[1]));
  const [launchSiteMessage, setLaunchSiteMessage] = useState("");
  const readerRef = useRef<ReadableStreamDefaultReader | null>(null); 
  const websocketRef = useRef<WebSocket | null>(null);
  const websocketBufferRef = useRef("");
  const streamStartTimeRef = useRef<number>(0);

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
      streamStartTimeRef.current = Date.now();

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

        // Decode each complete line into the shared telemetry shape.
        for (const line of lines) {
          const packet = dataSource === "srad"
            ? parseSradTelemetry(line)
            : parseCotsGpsTelemetry(
                line,
                Date.now() - streamStartTimeRef.current,
              );

          if (packet) {
            setTelemetryData((prev) => [...prev, packet]);
          }
        }
      }
    } catch (err) {
      console.error("Failed to connect:", err);
      setPortStatus(STATUS.DISCONNECTED);
    }
  }, [dataSource, selectedPort, setPortStatus, setTelemetryData]);

  const connectWebSocket = useCallback(() => {
    if (websocketRef.current) return;

    try {
      const socket = new WebSocket(websocketUrl);
      websocketRef.current = socket;
      websocketBufferRef.current = "";
      streamStartTimeRef.current = Date.now();

      socket.onopen = () => {
        setPortStatus(STATUS.CONNECTED);
      };

      socket.onmessage = (event) => {
        const text = typeof event.data === "string" ? event.data : "";
        if (!text) return;

        setRawData((previous) => previous + text);
        websocketBufferRef.current += text;
        const lines = websocketBufferRef.current.split(/\r?\n/);
        websocketBufferRef.current = lines.pop() || "";

        for (const line of lines) {
          const packet = dataSource === "srad"
            ? parseSradTelemetry(line)
            : parseCotsGpsTelemetry(
                line,
                Date.now() - streamStartTimeRef.current,
              );

          if (packet) {
            setTelemetryData((previous) => [...previous, packet]);
          }
        }
      };

      socket.onerror = (error) => {
        console.error("WebSocket error:", error);
        setPortStatus(STATUS.DISCONNECTED);
      };

      socket.onclose = () => {
        websocketRef.current = null;
        websocketBufferRef.current = "";
        setPortStatus(STATUS.DISCONNECTED);
      };
    } catch (error) {
      console.error("Failed to connect to WebSocket:", error);
      websocketRef.current = null;
      setPortStatus(STATUS.DISCONNECTED);
    }
  }, [dataSource, setPortStatus, setTelemetryData, websocketUrl]);

  // Closes the port
  const disconnectPort = useCallback(async () => {
    if (transport === "websocket") {
      websocketRef.current?.close();
      websocketRef.current = null;
      setPortStatus(STATUS.DISCONNECTED);
      return;
    }

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
  }, [selectedPort, setPortStatus, transport]);

  const applyLaunchSite = () => {
    const longitude = Number(launchLongitude);
    const latitude = Number(launchLatitude);

    if (
      !Number.isFinite(latitude) ||
      !Number.isFinite(longitude) ||
      latitude < -90 ||
      latitude > 90 ||
      longitude < -180 ||
      longitude > 180
    ) {
      setLaunchSiteMessage("Enter a valid latitude and longitude.");
      return;
    }

    setLaunchSite([longitude, latitude]);
    setLaunchSiteMessage("Launch site updated.");
  };

  const exportTelemetryCsv = () => {
    const columns: (keyof Telemetry)[] = [
      "time",
      "Temp",
      "pressure",
      "altitude",
      "accX",
      "accY",
      "accZ",
      "angVelX",
      "angVelY",
      "angVelZ",
      "lat",
      "lon",
    ];
    const csv = [
      columns.join(","),
      ...telemetryData.map((packet) =>
        columns.map((column) => packet[column]).join(",")
      ),
    ].join("\n");
    const filename = `telemetry-${new Date().toISOString().replace(/[:.]/g, "-")}.csv`;
    const blob = new Blob([csv], { type: "text/csv;charset=utf-8" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename;
    link.click();
    URL.revokeObjectURL(url);
  };

  useEffect(() => {
    if (selectedPort) {
      setPortStatus(STATUS.AWAITING);
    }
  }, [selectedPort, setPortStatus]);

  return (
    <div className="pt-8 px-4 sm:px-8 md:px-16">
      <div className="mb-6 rounded bg-slate-900/70 p-4 text-white">
        <p className="mb-3 font-semibold">Launch site</p>
        <div className="grid grid-cols-1 gap-3 sm:grid-cols-2">
          <label className="text-sm">
            Latitude
            <input
              type="number"
              min="-90"
              max="90"
              step="any"
              value={launchLatitude}
              onChange={(event) => setLaunchLatitude(event.target.value)}
              className="mt-1 w-full rounded border border-slate-400 px-3 py-2 text-black"
            />
          </label>
          <label className="text-sm">
            Longitude
            <input
              type="number"
              min="-180"
              max="180"
              step="any"
              value={launchLongitude}
              onChange={(event) => setLaunchLongitude(event.target.value)}
              className="mt-1 w-full rounded border border-slate-400 px-3 py-2 text-black"
            />
          </label>
        </div>
        <Button
          type="button"
          onClick={applyLaunchSite}
          className="mt-3 bg-yellow-500 text-white hover:bg-yellow-600"
        >
          Apply launch site
        </Button>
        {launchSiteMessage && (
          <p className="mt-2 text-sm text-slate-300">{launchSiteMessage}</p>
        )}
      </div>

      <div className="mb-4">
        <p className="mb-2 font-semibold text-white">Connection type</p>
        <div className="flex flex-col gap-2 sm:flex-row">
          <Button
            type="button"
            onClick={() => setTransport("serial")}
            disabled={isConnected}
            className={transport === "serial"
              ? "bg-yellow-500 text-white hover:bg-yellow-600"
              : "bg-gray-700 text-white hover:bg-gray-800"}
          >
            Serial
          </Button>
          <Button
            type="button"
            onClick={() => setTransport("websocket")}
            disabled={isConnected}
            className={transport === "websocket"
              ? "bg-yellow-500 text-white hover:bg-yellow-600"
              : "bg-gray-700 text-white hover:bg-gray-800"}
          >
            WebSocket
          </Button>
        </div>
        {transport === "websocket" && (
          <input
            type="text"
            value={websocketUrl}
            onChange={(event) => setWebsocketUrl(event.target.value)}
            disabled={isConnected}
            aria-label="WebSocket URL"
            className="mt-2 w-full rounded border border-slate-400 px-3 py-2 text-black"
            placeholder="ws://localhost:8765"
          />
        )}
      </div>

      <div className="mb-4">
        <p className="mb-2 font-semibold text-white">Data source</p>
        <div className="flex flex-col gap-2 sm:flex-row">
          <Button
            type="button"
            onClick={() => setDataSource("srad")}
            disabled={isConnected}
            className={dataSource === "srad"
              ? "bg-yellow-500 text-white hover:bg-yellow-600"
              : "bg-gray-700 text-white hover:bg-gray-800"}
          >
            SRAD
          </Button>
          <Button
            type="button"
            onClick={() => setDataSource("cots")}
            disabled={isConnected}
            className={dataSource === "cots"
              ? "bg-yellow-500 text-white hover:bg-yellow-600"
              : "bg-gray-700 text-white hover:bg-gray-800"}
          >
            COTS Feather
          </Button>
        </div>
        <p className="mt-2 text-sm text-gray-300">
          {dataSource === "srad"
            ? "12-field CSV telemetry"
            : "GPS_STAT position telemetry; unavailable sensors are zero"}
        </p>
      </div>

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
            {ports.map((port, i) => {
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
          onClick={exportTelemetryCsv}
          disabled={telemetryData.length === 0}
          className="bg-blue-700 hover:bg-blue-800 w-full sm:w-auto"
        >
          Export CSV
        </Button>
        <Button
          // onClick={isConnected ? disconnectPort : startMockTelemetry} // NOTE: for mock data
          onClick={isConnected
            ? disconnectPort
            : transport === "websocket"
              ? connectWebSocket
              : connectPort}
          disabled={transport === "serial" && !selectedPort}
          className={`text-white w-full sm:w-auto ${
            transport === "websocket" || selectedPort
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
