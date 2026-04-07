/* eslint-disable @next/next/no-img-element */
"use client";

import { motion } from "framer-motion";

// Anchor positions expressed as percentages of the homemap.svg viewBox,
// distributed across the floor plan so up to 9 cameras can be placed.
const ANCHOR_POSITIONS = [
  { x: 5, y: 90 },
  { x: 50, y: 20 },
  { x: 80, y: 25 },
  { x: 18, y: 52 },
  { x: 50, y: 50 },
  { x: 82, y: 52 },
  { x: 22, y: 78 },
  { x: 50, y: 80 },
  { x: 78, y: 78 },
];

// Frontend-only camera label overrides.
// Update these values to rename cameras in the map panel UI.
const CAMERA_LABEL_OVERRIDES = {
  cam1: "Main",
  cam2: "Pantry",
};

function formatCameraId(id) {
  if (!id) return "";

  return id
    .replace(/[_-]+/g, " ")
    .replace(/([a-zA-Z])(\d+)/g, "$1 $2")
    .replace(/\s+/g, " ")
    .trim()
    .replace(/\b[a-z]/g, (ch) => ch.toUpperCase());
}

function getCameraLabel(device) {
  return CAMERA_LABEL_OVERRIDES[device.id] || formatCameraId(device.label || device.id);
}

function CameraNode({ device, position, isSelected, onSelect }) {
  const label = getCameraLabel(device);
  const displayLabel = label.length > 10 ? `${label.slice(0, 10)}...` : label;

  return (
    <button
      type="button"
      onClick={() => onSelect(device.id)}
      className="group absolute flex -translate-x-1/2 -translate-y-1/2 flex-col items-center"
      style={{ left: `${position.x}%`, top: `${position.y}%` }}
    >
      {/* Pulsing glow ring for selected */}
      {isSelected && (
        <motion.span
          animate={{ scale: [1, 1.4, 1], opacity: [0.8, 0.2, 0.8] }}
          transition={{ duration: 2, repeat: Infinity }}
          className="absolute h-5 w-5 rounded-full border border-green-400"
        />
      )}

      {/* Node dot */}
      <span
        className={`h-3 w-3 rounded-full border transition-colors ${
          isSelected
            ? "border-green-300 bg-green-400 shadow-[0_0_8px_rgba(0,255,65,0.8)]"
            : "border-green-600 bg-green-900 group-hover:bg-green-700"
        }`}
      />

      {/* Label */}
      <span
        className={`mt-1 whitespace-nowrap rounded bg-black/70 px-1 py-[1px] font-mono text-[9px] tracking-wide backdrop-blur-sm ${
          isSelected ? "text-glow-green text-green-300" : "text-green-600 group-hover:text-green-400"
        }`}
      >
        {displayLabel}
      </span>
    </button>
  );
}

export default function CameraMapPanel({ devices, selectedCamera, onCameraSelect }) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 40, scale: 0.95 }}
      animate={{ opacity: 1, y: 0, scale: 1 }}
      exit={{ opacity: 0, y: 40, scale: 0.95 }}
      transition={{ type: "spring", damping: 25, stiffness: 300 }}
      className="fixed bottom-[calc(5vh+3.5rem)] right-[5vw] z-20 w-[40vw] min-w-[400px] max-w-[640px] overflow-hidden rounded-xl border border-green-800/40 bg-gray-950/90 backdrop-blur-md"
    >
      {/* Title bar */}
      <div className="border-b border-green-800/40 px-3 py-2">
        <p className="text-glow-green text-center font-mono text-xs uppercase tracking-[0.3em] text-green-400">
          Camera System
        </p>
      </div>

      {/* Map container with floor plan background + overlay nodes */}
      <div className="relative aspect-[1843/1024] w-full">
        <img
          src="/homemap.svg"
          alt="Floor plan"
          className="absolute inset-0 h-full w-full object-contain opacity-80"
          draggable={false}
        />

        {/* Green tint overlay to match the HUD aesthetic */}
        <div className="pointer-events-none absolute inset-0 bg-green-950/20 mix-blend-multiply" />

        {/* Camera nodes */}
        {devices.map((device, index) => {
          const position = ANCHOR_POSITIONS[index % ANCHOR_POSITIONS.length];
          return (
            <CameraNode
              key={device.id}
              device={device}
              position={position}
              isSelected={device.id === selectedCamera}
              onSelect={onCameraSelect}
            />
          );
        })}

        {/* Scanline overlay */}
        <div className="scanline-overlay scanline-sweep pointer-events-none absolute inset-0" />
      </div>
    </motion.div>
  );
}
