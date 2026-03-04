"use client";

import { useEffect, useRef } from "react";

export default function useMjpegStream(cameraId) {
  const imgRef = useRef(null);

  useEffect(() => {
    const streamBaseUrl =
      (process.env.NEXT_PUBLIC_STREAM_URL || "/api").replace(/\/+$/, "");
    const img = imgRef.current;
    if (!img) return;

    let retryTimer = null;
    const streamUrl = `${streamBaseUrl}/mjpeg/${cameraId}`;

    const connect = () => {
      img.src = `${streamUrl}?t=${Date.now()}`;
    };

    const onError = () => {
      if (retryTimer) clearTimeout(retryTimer);
      retryTimer = setTimeout(connect, 1500);
    };

    img.addEventListener("error", onError);
    connect();

    return () => {
      img.removeEventListener("error", onError);
      if (retryTimer) clearTimeout(retryTimer);
    };
  }, [cameraId]);

  return imgRef;
}
