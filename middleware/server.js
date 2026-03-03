const express = require("express");
const app = express();
const PORT = process.env.PORT || 8080;

app.get("/health", (req, res) => {
  res.json({ status: "ok", service: "edr-middleware" });
});

app.listen(PORT, () => {
  console.log(`Middleware placeholder listening on port ${PORT}`);
});
