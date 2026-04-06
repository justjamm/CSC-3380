"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { login } from "../lib/apiClient";
import { useAuth } from "../providers/AuthProvider";

export default function LoginPage() {
  const router = useRouter();
  const { isAuthenticated, isLoading: authLoading, startOtpFlow } = useAuth();

  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [otpHint, setOtpHint] = useState("");

  useEffect(() => {
    if (!authLoading && isAuthenticated) {
      router.replace("/");
    }
  }, [authLoading, isAuthenticated, router]);

  const onSubmit = async (event) => {
    event.preventDefault();
    setError("");
    setSubmitting(true);

    try {
      const response = await login(username.trim(), password);
      startOtpFlow(username.trim());
      setOtpHint(response?.otp || "");
      router.push("/verify-otp");
    } catch (err) {
      setError(err.message || "Login failed");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <main className="mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
      <h1 className="mb-2 text-2xl font-semibold">Sign in</h1>
      <p className="mb-6 text-sm text-gray-300">Use your middleware credentials to start OTP flow.</p>

      <form onSubmit={onSubmit} className="space-y-4 rounded-xl border border-gray-700 bg-black/30 p-5">
        <label className="block text-sm">
          Username
          <input
            value={username}
            onChange={(event) => setUsername(event.target.value)}
            className="mt-1 w-full rounded-md border border-gray-700 bg-black px-3 py-2"
            autoComplete="username"
            required
          />
        </label>

        <label className="block text-sm">
          Password
          <input
            type="password"
            value={password}
            onChange={(event) => setPassword(event.target.value)}
            className="mt-1 w-full rounded-md border border-gray-700 bg-black px-3 py-2"
            autoComplete="current-password"
            required
          />
        </label>

        {error ? <p className="text-sm text-red-400">{error}</p> : null}
        {otpHint ? <p className="text-xs text-yellow-300">Dev OTP hint: {otpHint}</p> : null}

        <button
          type="submit"
          disabled={submitting}
          className="w-full rounded-md bg-white px-4 py-2 font-medium text-black disabled:opacity-60"
        >
          {submitting ? "Signing in..." : "Continue to OTP"}
        </button>
      </form>
    </main>
  );
}
