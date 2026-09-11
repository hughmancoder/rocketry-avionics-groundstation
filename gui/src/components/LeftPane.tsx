import { STATUS, Metric as TMetric, Telemetry } from "../types";
import { Metric } from "./Metric";

const iconColors: Record<STATUS, string> = {
  connected: "text-green",
  awaiting: "text-yellow",
  disconnected: "text-gray-600",
};


const labelColors: Record<STATUS, string> = {
  connected: "text-white",
  awaiting: "text-white",
  disconnected: "text-gray-600/50",
};


const getIconColor = (status: STATUS): string => iconColors[status];
const getLabelColor = (status: STATUS): string => labelColors[status]; 


interface Props {
  data: Telemetry;
  serial_status: STATUS;
  telemetry_status: STATUS;
}

const LeftPane = ({ data, serial_status, telemetry_status } : Props) => {

  const connectionData = [
    { title: "Serial Port", status: serial_status },
    { title: "Sensor Status", status: telemetry_status },
  ];

  const metrics: TMetric[] = [
    { title: "Temperature", value: data.Temp.toFixed(2), unit: "°C" },
    { title: "Pressure", value: data.pressure.toFixed(2), unit: "hPa" },
    { title: "Latitude", value: data.lat.toFixed(6), unit: "°" },
    { title: "Longitude", value: data.lon.toFixed(6), unit: "°" },
  ];

  return (
    <div className="w-full max-w-xs space-y-8">
      <div className="space-y-11">
        {metrics.map((metric) => (
          <Metric key={metric.title} metrics={metric}></Metric>
        ))}
      </div>

      <div className="space-y-11">
        {connectionData.map((connection, index) => (
          <div key={`{connection}-${index}`} className="flex items-center">
            {/* Icon */}
            <svg
              className={`w-6 h-6 fill-current ${getIconColor(connection.status)}`}
              viewBox="0 0 24 24"
              xmlns="http://www.w3.org/2000/svg"
            >
              <path
                fillRule="evenodd"
                clipRule="evenodd"
                d="M12 24C18.6274 24 24 18.6274 24 12C24 5.37258 18.6274 0 12 0C5.37258 0 0 5.37258 0 12C0 18.6274 5.37258 24 12 24ZM17.6647 10.1879C18.0446 9.78705 18.0276 9.15411 17.6268 8.77419C17.2259 8.39428 16.593 8.41125 16.2131 8.8121L10.8574 14.463L8.81882 12.3121C8.43891 11.9112 7.80597 11.8943 7.40512 12.2742C7.00427 12.6541 6.9873 13.287 7.36721 13.6879L10.1315 16.6046C10.3204 16.8038 10.5828 16.9167 10.8574 16.9167C11.1319 16.9167 11.3943 16.8038 11.5832 16.6046L17.6647 10.1879Z"
              />
            </svg>

            {/* Connection Details */}
            <div className="flex flex-col pl-3">
              <span className="text-xs font-bold text-gray-600 uppercase whitespace-nowrap">
                {connection.title}
              </span>
              <span
                className={`text-xs capitalize ${getLabelColor(connection.status)}`}
              >
                {connection.status}
              </span>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};

export default LeftPane;
