"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { verifyOtp } from "../lib/apiClient";
import { useAuth } from "../providers/AuthProvider";

export default function VerifyOtpPage() {
  const router = useRouter();
  const { pendingEmail, completeAuth, isAuthenticated, isLoading: authLoading } = useAuth();

  const [otp, setOtp] = useState("");
  const [error, setError] = useState("");
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    if (!authLoading && isAuthenticated) {
      router.replace("/");
      return;
    }

    if (!authLoading && !pendingEmail) {
      router.replace("/login");
    }
  }, [authLoading, isAuthenticated, pendingEmail, router]);

  const onSubmit = async (event) => {
    event.preventDefault();
    setError("");
    setSubmitting(true);

    try {
      const response = await verifyOtp(pendingEmail, otp.trim());
      completeAuth(response);
      router.replace("/");
    } catch (err) {
      setError(err.message || "OTP verification failed");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <main className="relative mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
      <p className="mb-2 font-display text-[10px] font-bold uppercase tracking-[0.4em] text-accent/60">
        {"// SECURE_ACCESS // TIER_02"}
      </p>
      <h1 className="hud-title mb-2 text-3xl">Verify</h1>
      <p className="mb-6 font-mono text-[11px] uppercase tracking-[0.2em] text-foreground/60">
        &gt; OTP dispatched to{" "}
        <span className="text-accent">{pendingEmail || "your email"}</span>
      </p>

      <form
        onSubmit={onSubmit}
        className="corner-bracket-full space-y-4 rounded-xl border border-accent/40 bg-hud-panel p-5 inset-shadow-sm inset-shadow-accent/15"
      >
        <label className="block font-display text-[10px] font-bold uppercase tracking-[0.3em] text-accent">
          One-time password
          <input
            value={otp}
            onChange={(event) => setOtp(event.target.value)}
            className="hud-input mt-2 w-full rounded-md px-3 py-2 text-center text-lg tracking-[0.5em]"
            inputMode="numeric"
            autoComplete="one-time-code"
            required
          />
        </label>

        {error ? (
          <p className="text-glow-danger font-mono text-[11px] uppercase tracking-wider text-danger">
            ! {error}
          </p>
        ) : null}

        <button
          type="submit"
          disabled={submitting}
          className="hud-button w-full rounded-md px-4 py-2 text-sm"
        >
          {submitting ? "Verifying..." : "Verify >> Access"}
        </button>
      </form>
    </main>
  );
}
