/* eslint-disable @next/next/no-img-element */
"use client";

import { motion } from "framer-motion";

const ANCHOR_POSITIONS = [
  { x: 5, y: 90 },
  { x: 50, y: 20 },
  { x: 5, y: 45 }
];

const CAMERA_LABEL_OVERRIDES = {
  cam1: "Main",
  cam2: "Pantry",
  cam3: "Hall"
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
      {isSelected && (
        <motion.span
          animate={{ scale: [1, 1.6, 1], opacity: [0.8, 0.1, 0.8] }}
          transition={{ duration: 2, repeat: Infinity }}
          className="absolute h-4 w-4 rounded-full border border-accent sm:h-5 sm:w-5"
          style={{ boxShadow: "0 0 12px var(--color-accent)" }}
        />
      )}

      <span
        className={`h-2.5 w-2.5 rounded-full border transition-colors sm:h-3 sm:w-3 ${
          isSelected
            ? "border-accent bg-accent shadow-[0_0_10px_var(--color-accent)]"
            : "border-accent/60 bg-accent/30 group-hover:bg-accent/60"
        }`}
      />

      <span
        className={`mt-1 whitespace-nowrap rounded-sm border px-1 py-[1px] font-display text-[8px] font-bold uppercase tracking-[0.15em] backdrop-blur-sm sm:text-[9px] ${
          isSelected
            ? "border-accent/60 bg-black/80 text-accent text-glow"
            : "border-accent/20 bg-black/70 text-accent/60 group-hover:text-accent"
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
      className="fixed right-3 bottom-[calc(env(safe-area-inset-bottom)+4.5rem)] left-3 z-20 max-h-[62vh] overflow-hidden rounded-xl border border-accent/40 bg-hud-panel backdrop-blur-md inset-shadow-sm inset-shadow-accent/15 sm:right-[5vw] sm:bottom-[calc(5vh+3.5rem)] sm:left-auto sm:max-h-none sm:w-[40vw] sm:min-w-[400px] sm:max-w-[640px]"
    >
      <div className="relative overflow-hidden border-b border-accent/30 px-3 py-2">
        <div className="radar-sweep" aria-hidden />
        <p className="hud-title relative text-center text-[11px] sm:text-xs">
          {"// CAMERA_SYSTEM"}
        </p>
      </div>

      <div className="relative aspect-[1843/1024] w-full">
        <img
          src="/homemap.svg"
          alt="Floor plan"
          className="absolute inset-0 h-full w-full object-contain opacity-70"
          style={{ filter: "hue-rotate(0deg)" }}
          draggable={false}
        />

        <div
          className="pointer-events-none absolute inset-0 mix-blend-multiply"
          style={{ background: "color-mix(in srgb, var(--color-accent) 14%, transparent)" }}
        />

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

        <div className="scanline-overlay scanline-sweep pointer-events-none absolute inset-0" />
      </div>
    </motion.div>
  );
}
