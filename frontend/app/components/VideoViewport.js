/* eslint-disable @next/next/no-img-element */
"use client";

export default function VideoViewport({ imgRef, selectedCamera, loading }) {
  return (
    <div className="fixed inset-0 z-0 bg-black">
      <div className="relative m-2 h-[calc(100dvh-1rem)] w-[calc(100vw-1rem)] overflow-hidden rounded-md border border-gray-800 bg-black sm:mx-[5vw] sm:my-[5vh] sm:h-[90vh] sm:w-[90vw] sm:rounded-lg">
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
