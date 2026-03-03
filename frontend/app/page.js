"use client";

import { useEffect, useRef } from "react";

export default function Home() {
  const imgRef = useRef(null);

  useEffect(() => {
    const streamBaseUrl =
      (process.env.NEXT_PUBLIC_STREAM_URL || "/api").replace(/\/+$/, "");
    const img = imgRef.current;
    if (!img) return;

    img.src = `${streamBaseUrl}/mjpeg`;
  }, []);

  return (
    <main className="flex min-h-screen flex-col items-center p-8">
      <h1 className="text-3xl font-bold mb-8">IoT EDR Hub</h1>
      <div className="w-full max-w-3xl">
        <img
          ref={imgRef}
          alt="Camera Stream"
          className="w-full rounded-lg border border-gray-700"
        />
      </div>
    </main>
  );
}
