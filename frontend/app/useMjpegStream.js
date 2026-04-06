"use client";

import { useEffect, useRef } from "react";
import { getStreamFrame } from "./lib/apiClient";

export default function useMjpegStream(cameraId, accessToken) {
  const imgRef = useRef(null);

  useEffect(() => {
    const img = imgRef.current;
    if (!img) {
      return;
    }
    if (!cameraId || !accessToken) {
      img.removeAttribute("src");
      return;
    }

    let retryTimer = null;
    let active = true;
    let currentObjectUrl = "";
    let currentRequest = null;

    const loadFrame = async () => {
      currentRequest = new AbortController();
      try {
        const blob = await getStreamFrame(cameraId, accessToken, currentRequest.signal);
        if (!active) {
          return;
        }

        const nextObjectUrl = URL.createObjectURL(blob);
        if (currentObjectUrl) {
          URL.revokeObjectURL(currentObjectUrl);
        }
        currentObjectUrl = nextObjectUrl;
        img.src = currentObjectUrl;

        retryTimer = setTimeout(loadFrame, 900);
      } catch {
        if (!active) {
          return;
        }
        retryTimer = setTimeout(loadFrame, 1500);
      }
    };

    loadFrame();

    return () => {
      active = false;
      if (retryTimer) {
        clearTimeout(retryTimer);
      }
      if (currentRequest) {
        currentRequest.abort();
      }
      if (currentObjectUrl) {
        URL.revokeObjectURL(currentObjectUrl);
      }
    };
  }, [cameraId, accessToken]);

  return imgRef;
}
