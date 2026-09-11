import { useEffect, useRef, useState } from "react";
import * as maplibregl from "maplibre-gl";
import { Map as MapLibreMap, type LngLatLike } from "maplibre-gl";
import "maplibre-gl/dist/maplibre-gl.css";
import { Telemetry } from "@/types";

const LAUNCH_SITE: [number, number] = [143.189907, -30.671004];
const MAX_MAP_POINTS = 1000;
const MAP_BASE_URL = `${import.meta.env.BASE_URL}maps`;
const MAP_BOUNDS: [number, number, number, number] = [
  143.146306,
  -30.714806,
  143.246306,
  -30.614806,
];

const offlineStyle: maplibregl.StyleSpecification = {
  version: 8,
  sources: {
    satellite: {
      type: "raster",
      tiles: [`${MAP_BASE_URL}/tiles/{z}/{x}/{y}.jpg`],
      tileSize: 256,
      bounds: MAP_BOUNDS,
      attribution: "Satellite imagery: local launch-site dataset",
    },
  },
  layers: [
    {
      id: "satellite",
      type: "raster",
      source: "satellite",
      paint: { "raster-opacity": 1 },
    },
  ],
};

function isValidPoint(point: Telemetry) {
  return (
    Number.isFinite(point.lat) &&
    Number.isFinite(point.lon) &&
    point.lat >= -90 &&
    point.lat <= 90 &&
    point.lon >= -180 &&
    point.lon <= 180 &&
    (point.lat !== 0 || point.lon !== 0)
  );
}

function FlightPathOverlay({ map, points }: { map: MapLibreMap; points: Telemetry[] }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const draw = () => {
      const bounds = map.getContainer().getBoundingClientRect();
      const pixelRatio = window.devicePixelRatio || 1;
      canvas.width = bounds.width * pixelRatio;
      canvas.height = bounds.height * pixelRatio;
      canvas.style.width = `${bounds.width}px`;
      canvas.style.height = `${bounds.height}px`;

      const context = canvas.getContext("2d");
      if (!context) return;
      context.setTransform(pixelRatio, 0, 0, pixelRatio, 0, 0);
      context.clearRect(0, 0, bounds.width, bounds.height);

      if (points.length === 0) return;

      const pixelsPerMeter =
        (256 * 2 ** map.getZoom()) /
        (40075017 * Math.cos((map.getCenter().lat * Math.PI) / 180));
      const altitudeScale = pixelsPerMeter * 0.45;
      const ground = points.map((point) => map.project([point.lon, point.lat]));
      const elevated = points.map((point, index) => ({
        x: ground[index].x,
        y: ground[index].y - Math.max(0, point.altitude) * altitudeScale,
      }));

      context.lineWidth = 2;
      context.strokeStyle = "rgba(255, 255, 255, 0.65)";
      context.setLineDash([5, 5]);
      context.beginPath();
      ground.forEach((point, index) => {
        if (index === 0) context.moveTo(point.x, point.y);
        else context.lineTo(point.x, point.y);
      });
      context.stroke();
      context.setLineDash([]);

      context.lineWidth = 4;
      context.strokeStyle = "#ffbd2e";
      context.beginPath();
      elevated.forEach((point, index) => {
        if (index === 0) context.moveTo(point.x, point.y);
        else context.lineTo(point.x, point.y);
      });
      context.stroke();

      context.lineWidth = 1;
      context.strokeStyle = "rgba(255, 189, 46, 0.7)";
      elevated.forEach((point, index) => {
        context.beginPath();
        context.moveTo(ground[index].x, ground[index].y);
        context.lineTo(point.x, point.y);
        context.stroke();
      });

      const latest = elevated[elevated.length - 1];
      context.fillStyle = "#ff4d4d";
      context.beginPath();
      context.arc(latest.x, latest.y, 7, 0, Math.PI * 2);
      context.fill();
      context.strokeStyle = "white";
      context.lineWidth = 2;
      context.stroke();
    };

    map.on("render", draw);
    draw();
    return () => {
      map.off("render", draw);
    };
  }, [map, points]);

  return <canvas ref={canvasRef} className="pointer-events-none absolute inset-0 z-10" />;
}

export default function MapPage({ data }: { data: Telemetry[] }) {
  const mapContainerRef = useRef<HTMLDivElement>(null);
  const mapRef = useRef<MapLibreMap | null>(null);
  const [map, setMap] = useState<MapLibreMap | null>(null);
  const [hasMapTiles, setHasMapTiles] = useState(true);
  const mapPoints = data.filter(isValidPoint).slice(-MAX_MAP_POINTS);

  useEffect(() => {
    if (!mapContainerRef.current || mapRef.current) return;

    const instance = new maplibregl.Map({
      container: mapContainerRef.current,
      style: offlineStyle,
      center: LAUNCH_SITE,
      zoom: 13,
      pitch: 55,
      bearing: -20,
      maxPitch: 75,
      maxBounds: MAP_BOUNDS,
    });
    instance.addControl(new maplibregl.NavigationControl(), "top-right");
    console.info("[Map] satellite tiles:", `${MAP_BASE_URL}/tiles/{z}/{x}/{y}.jpg`);
    instance.on("error", (event) => {
      console.error("[MapLibre] map error event:", event);
      console.error("[MapLibre] underlying error:", event.error ?? event);
      setHasMapTiles(false);
    });
    instance.once("load", () => {
      console.info("[Map] style loaded successfully");
      setMap(instance);
    });
    mapRef.current = instance;

    return () => {
      instance.remove();
      mapRef.current = null;
    };
  }, []);

  const followLatest = () => {
    const latest = mapPoints[mapPoints.length - 1];
    if (!latest || !map) return;
    map.easeTo({ center: [latest.lon, latest.lat] as LngLatLike, duration: 500 });
  };

  const viewPath = () => {
    if (!map || mapPoints.length === 0) return;
    const coordinates = mapPoints.map((point) => [point.lon, point.lat] as [number, number]);
    const bounds = coordinates.reduce(
      (result, coordinate) => result.extend(coordinate),
      new maplibregl.LngLatBounds(coordinates[0], coordinates[0]),
    );
    map.fitBounds(bounds, { padding: 80, pitch: 55, duration: 700, maxZoom: 16 });
  };

  return (
    <main className="relative h-full min-h-[34rem] overflow-hidden rounded-lg bg-slate-950">
      <div className="absolute inset-0">
        <div ref={mapContainerRef} className="h-full w-full" />
      </div>
      {map && <FlightPathOverlay map={map} points={mapPoints} />}

      <div className="pointer-events-none absolute left-4 top-4 z-20 max-w-xs rounded bg-slate-950/85 px-4 py-3 text-white shadow-lg">
        <p className="text-sm font-semibold">Flight map</p>
        <p className="mt-1 text-xs text-slate-300">
          {mapPoints.length} of {MAX_MAP_POINTS} map points
        </p>
        {!hasMapTiles && (
          <p className="mt-2 text-xs text-amber-300">
            Add local tiles at public/maps/tiles/&#123;z&#125;/&#123;x&#125;/&#123;y&#125;.jpg for offline imagery.
          </p>
        )}
      </div>

      <div className="absolute bottom-4 left-4 z-20 flex gap-2">
        <button className="rounded bg-slate-950/90 px-3 py-2 text-xs font-semibold text-white" onClick={followLatest}>
          Follow rocket
        </button>
        <button className="rounded bg-slate-950/90 px-3 py-2 text-xs font-semibold text-white" onClick={viewPath}>
          View path
        </button>
      </div>
    </main>
  );
}
