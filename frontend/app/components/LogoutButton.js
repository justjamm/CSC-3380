"use client";

export default function LogoutButton({ onLogout }) {
  return (
    <button
      type="button"
      onClick={onLogout}
      className="text-glow-green fixed bottom-[calc(env(safe-area-inset-bottom)+0.75rem)] left-3 z-30 m-2 min-h-11 rounded-lg border border-red-700/50 bg-red-900/50 px-3 py-2 font-mono text-[11px] uppercase tracking-wider text-red-300 backdrop-blur-sm transition-colors hover:bg-red-800/70 sm:right-[5vw] sm:bottom-[0.5vh] sm:left-auto sm:m-0 sm:min-h-0 sm:px-4 sm:text-xs"
    >
      Logout
    </button>
  );
}
