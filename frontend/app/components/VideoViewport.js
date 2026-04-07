/* eslint-disable @next/next/no-img-element */
"use client";

export default function VideoViewport({ imgRef, selectedCamera, loading }) {
  return (
    <div className="fixed inset-0 z-0 bg-black">
      <div className="relative mx-[5vw] my-[5vh] h-[90vh] w-[90vw] overflow-hidden rounded-lg border border-gray-800 bg-black">
        <img
          ref={imgRef}
          alt={`Camera stream – ${selectedCamera || "none"}`}
          className="h-full w-full object-contain"
        />

        {loading && (
          <div className="pointer-events-none absolute inset-0 animate-pulse bg-gray-900" />
        )}

        {/* Scanline overlay */}
        <div className="scanline-overlay scanline-sweep pointer-events-none absolute inset-0 z-10" />
      </div>
    </div>
  );
}
