"use client";

import { useState } from "react";
import useMjpegStream from "./useMjpegStream";

const cameras = [
  { id: "cam1", label: "Living Room" },
  { id: "cam2", label: "Pantry" },
];

export default function Home() {
  const [selectedCamera, setSelectedCamera] = useState("cam1");
  const imgRef = useMjpegStream(selectedCamera);

  return (
    <main className="flex min-h-screen flex-col items-center p-8">
      <h1 className="text-3xl font-bold mb-8">IoT EDR Hub</h1>
      <div className="w-full max-w-3xl">
        <select
          value={selectedCamera}
          onChange={(e) => setSelectedCamera(e.target.value)}
          className="mb-4 w-full rounded-lg border border-gray-700 bg-black px-4 py-2 text-white"
        >
          {cameras.map((cam) => (
            <option key={cam.id} value={cam.id}>
              {cam.label}
            </option>
          ))}
        </select>
        <img
          ref={imgRef}
          alt="Camera Stream"
          className="w-full rounded-lg border border-gray-700"
        />
      </div>
    </main>
  );
}
