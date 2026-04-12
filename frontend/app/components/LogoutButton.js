"use client";

export default function LogoutButton({ onLogout }) {
  return (
    <button
      type="button"
      onClick={onLogout}
      className="text-glow-green fixed bottom-[calc(env(safe-area-inset-bottom)+0.75rem)] left-3 z-30 min-h-11 rounded-lg border border-red-700/50 bg-red-900/50 px-3 py-2 font-mono text-[11px] uppercase tracking-wider text-red-300 backdrop-blur-sm transition-colors hover:bg-red-800/70 sm:bottom-[5vh] sm:left-[5vw] sm:min-h-0 sm:px-4 sm:text-xs"
    >
      Logout
    </button>
  );
}
