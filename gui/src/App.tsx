import { useState } from "react";
import LeftPane from "./components/LeftPane";
import RightPane from "./components/RightPane";
import { DEFAULT_TELEMETRY_DATA, PAGE, STATUS, Telemetry } from "./types";
import TelemetryPage from "./pages/TelemetryPage";
import SettingsPage from "./pages/SettingsPage";
import GraphPage from "./pages/GraphPage";
import MapPage from "./pages/MapPage";
import { DEFAULT_CONFIG } from "./config";

import backgroundImageFile from '@/assets/background-cropped.png';

function App() {
  // Shared data
  const [page, setPage] = useState<PAGE>(PAGE.TELEMETRY);
  const [portStatus, setPortStatus] = useState<STATUS>(STATUS.DISCONNECTED);
  const [telemetryData, setTelemetryData] = useState<Telemetry[]>([
    DEFAULT_TELEMETRY_DATA,
  ]);
  const [launchSite, setLaunchSite] = useState<[number, number]>([
    DEFAULT_CONFIG.launchSite.longitude,
    DEFAULT_CONFIG.launchSite.latitude,
  ]);

  const latest =
    telemetryData[telemetryData.length - 1] || DEFAULT_TELEMETRY_DATA;
  const rotationAngle = latest.angVelZ || 0;
  const time = latest.time || 0;
  const altitude = latest.altitude || 0;

  const settingsView = (
    <SettingsPage
      portStatus={portStatus}
      setPortStatus={setPortStatus}
      telemetryData={telemetryData}
      setTelemetryData={setTelemetryData}
      launchSite={launchSite}
      setLaunchSite={setLaunchSite}
    />
  );

  const renderView = () => {
    switch (page) {
      case PAGE.SETTINGS:
        return null;
      case PAGE.GRAPHS:
        return <GraphPage data={telemetryData} />;
      case PAGE.MAP:
        return <MapPage data={telemetryData} launchSite={launchSite} />;
      default:
        return (
          <TelemetryPage
            rotationAngle={rotationAngle}
            time={time}
            altitude={altitude}
          />
        );
    }
  };

  return (
    <div className="w-full h-screen font-sans bg-cover bg-gradient-to-b from-blue-950  bg-blue-900"
    style={{
      backgroundImage:
        page === PAGE.TELEMETRY ? `url(${backgroundImageFile})` : "none",
        backgroundPosition: "-10rem center",
      backgroundRepeat: "repeat", 
    }}
    >
      <div className="absolute inset-0 pointer-events-none bg-gradient-to-b from-black/30 to-transparent" />
      
      
      <header className="fixed top-0 left-0 right-0 shadow px-8 lg:px-16 py-8 z-10">
        <nav className="flex flex-row space-x-16">
          {Object.values(PAGE).map((item) => (
            <div
              key={item}
              onClick={() => setPage(item as PAGE)}
              className={`text-md font-bold rounded cursor-pointer hover:text-blue-100 ${
                page === item ? "text-white" : "text-gray-300"
              }`}
            >
              {item.charAt(0).toUpperCase() + item.slice(1)}
            </div>
          ))}
        </nav>
      </header>

      <div
        className="relative w-full h-full pt-16"
      >
        <div className="flex flex-row h-full">
          <div className="px-8 lg:px-16 py-8">
            <LeftPane data={latest} serial_status={portStatus} telemetry_status={STATUS.DISCONNECTED} />
          </div>
          <div className="flex-1 px-4 sm:px-6 md:px-8 overflow-y-auto">
            <div className={page === PAGE.SETTINGS ? "contents" : "hidden"}>
              {settingsView}
            </div>
            {renderView()}
          </div>
          <div className="px-8 lg:px-16 py-8">
            <RightPane data={latest} />
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
