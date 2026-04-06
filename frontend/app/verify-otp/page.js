"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { verifyOtp } from "../lib/apiClient";
import { useAuth } from "../providers/AuthProvider";

export default function VerifyOtpPage() {
  const router = useRouter();
  const { pendingUsername, completeAuth, isAuthenticated, isLoading: authLoading } = useAuth();

  const [otp, setOtp] = useState("");
  const [error, setError] = useState("");
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    if (!authLoading && isAuthenticated) {
      router.replace("/");
      return;
    }

    if (!authLoading && !pendingUsername) {
      router.replace("/login");
    }
  }, [authLoading, isAuthenticated, pendingUsername, router]);

  const onSubmit = async (event) => {
    event.preventDefault();
    setError("");
    setSubmitting(true);

    try {
      const response = await verifyOtp(pendingUsername, otp.trim());
      completeAuth(response);
      router.replace("/");
    } catch (err) {
      setError(err.message || "OTP verification failed");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <main className="mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
      <h1 className="mb-2 text-2xl font-semibold">Verify OTP</h1>
      <p className="mb-6 text-sm text-gray-300">Enter the OTP for {pendingUsername || "your account"}.</p>

      <form onSubmit={onSubmit} className="space-y-4 rounded-xl border border-gray-700 bg-black/30 p-5">
        <label className="block text-sm">
          One-time password
          <input
            value={otp}
            onChange={(event) => setOtp(event.target.value)}
            className="mt-1 w-full rounded-md border border-gray-700 bg-black px-3 py-2"
            inputMode="numeric"
            autoComplete="one-time-code"
            required
          />
        </label>

        {error ? <p className="text-sm text-red-400">{error}</p> : null}

        <button
          type="submit"
          disabled={submitting}
          className="w-full rounded-md bg-white px-4 py-2 font-medium text-black disabled:opacity-60"
        >
          {submitting ? "Verifying..." : "Verify OTP"}
        </button>
      </form>
    </main>
  );
}
