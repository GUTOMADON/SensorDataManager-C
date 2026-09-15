// SensorDataManager-C dashboard logic.
//
// Talks to the C backend over plain fetch() calls. Edit API_BASE_URL if the
// backend is not running on localhost:8080 (for example when deployed to a
// different host, or exposed on a different port).
const API_BASE_URL = "http://localhost:8080/api";

const SENSOR_LABELS = {
  temperature: "Temperature",
  vibration: "Vibration",
  rotational_speed: "Rotational speed",
};

function sensorLabel(sensorType) {
  return SENSOR_LABELS[sensorType] || sensorType;
}

function formatNumber(value) {
  if (value === null || value === undefined) return "";
  return Number(value).toFixed(2);
}

// Converts a "datetime-local" input value (no timezone) into an ISO 8601
// UTC string suitable for the API, assuming the browser's local timezone.
function localInputToIso(value) {
  if (!value) return null;
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return null;
  return date.toISOString();
}

async function apiRequest(path, options) {
  const response = await fetch(`${API_BASE_URL}${path}`, options);
  let data = null;
  try {
    data = await response.json();
  } catch (err) {
    data = null;
  }
  if (!response.ok) {
    const message = (data && data.error) ? data.error : `Request failed with status ${response.status}`;
    throw new Error(message);
  }
  return data;
}

function setStatus(ok, text) {
  const dot = document.getElementById("status-dot");
  const label = document.getElementById("status-text");
  dot.className = "status-dot " + (ok ? "ok" : "error");
  label.textContent = text;
}

async function checkHealth() {
  try {
    const data = await apiRequest("/health");
    setStatus(data.database === "connected", `API online, database ${data.database}`);
  } catch (err) {
    setStatus(false, "API unreachable");
  }
}

async function loadEquipment() {
  const selects = [
    document.getElementById("input-equipment"),
    document.getElementById("filter-equipment"),
    document.getElementById("stats-filter-equipment"),
  ];

  try {
    const data = await apiRequest("/equipment");
    const equipmentList = data.equipment || [];

    selects.forEach((select, index) => {
      const isFilter = index !== 0; // first select is the submit-form one, no "All" option
      select.innerHTML = "";
      if (isFilter) {
        const allOption = document.createElement("option");
        allOption.value = "";
        allOption.textContent = "All";
        select.appendChild(allOption);
      }
      equipmentList.forEach((equipment) => {
        const option = document.createElement("option");
        option.value = String(equipment.id);
        option.textContent = `${equipment.name} (${equipment.equipment_type})`;
        select.appendChild(option);
      });
    });
  } catch (err) {
    console.error("Failed to load equipment", err);
  }
}

function renderReadingsTable(readings) {
  const body = document.getElementById("readings-body");
  body.innerHTML = "";

  if (readings.length === 0) {
    body.innerHTML = '<tr><td colspan="5" class="empty-row">No readings found.</td></tr>';
    return;
  }

  readings.forEach((reading) => {
    const row = document.createElement("tr");
    row.innerHTML = `
      <td>${reading.recorded_at}</td>
      <td>${reading.equipment_name || reading.equipment_id}</td>
      <td><span class="sensor-badge ${reading.sensor_type}">${sensorLabel(reading.sensor_type)}</span></td>
      <td>${formatNumber(reading.value)}</td>
      <td>${reading.unit}</td>
    `;
    body.appendChild(row);
  });
}

async function loadReadings() {
  const body = document.getElementById("readings-body");
  body.innerHTML = '<tr><td colspan="5" class="empty-row">Loading...</td></tr>';

  const equipmentId = document.getElementById("filter-equipment").value;
  const sensorType = document.getElementById("filter-sensor-type").value;
  const from = document.getElementById("filter-from").value;
  const to = document.getElementById("filter-to").value;
  const limit = document.getElementById("filter-limit").value;

  const query = new URLSearchParams();
  if (equipmentId) query.set("equipment_id", equipmentId);
  if (sensorType) query.set("sensor_type", sensorType);
  const fromIso = localInputToIso(from);
  const toIso = localInputToIso(to);
  if (fromIso) query.set("from", fromIso);
  if (toIso) query.set("to", toIso);
  if (limit) query.set("limit", limit);

  try {
    const data = await apiRequest(`/readings?${query.toString()}`);
    renderReadingsTable(data.readings || []);
  } catch (err) {
    body.innerHTML = `<tr><td colspan="5" class="empty-row">Error: ${err.message}</td></tr>`;
  }
}

function renderStatsTable(stats) {
  const body = document.getElementById("stats-body");
  body.innerHTML = "";

  if (stats.length === 0) {
    body.innerHTML = '<tr><td colspan="6" class="empty-row">No statistics available.</td></tr>';
    return;
  }

  stats.forEach((stat) => {
    const row = document.createElement("tr");
    row.innerHTML = `
      <td>${stat.equipment_name}</td>
      <td><span class="sensor-badge ${stat.sensor_type}">${sensorLabel(stat.sensor_type)}</span></td>
      <td>${formatNumber(stat.average)}</td>
      <td>${formatNumber(stat.minimum)}</td>
      <td>${formatNumber(stat.maximum)}</td>
      <td>${stat.sample_count}</td>
    `;
    body.appendChild(row);
  });
}

async function loadStats() {
  const body = document.getElementById("stats-body");
  body.innerHTML = '<tr><td colspan="6" class="empty-row">Loading...</td></tr>';

  const equipmentId = document.getElementById("stats-filter-equipment").value;
  const sensorType = document.getElementById("stats-filter-sensor-type").value;

  const query = new URLSearchParams();
  if (equipmentId) query.set("equipment_id", equipmentId);
  if (sensorType) query.set("sensor_type", sensorType);

  try {
    const data = await apiRequest(`/stats?${query.toString()}`);
    renderStatsTable(data.stats || []);
  } catch (err) {
    body.innerHTML = `<tr><td colspan="6" class="empty-row">Error: ${err.message}</td></tr>`;
  }
}

async function submitReading(event) {
  event.preventDefault();

  const feedback = document.getElementById("submit-feedback");
  feedback.textContent = "";
  feedback.className = "feedback";

  const equipmentId = document.getElementById("input-equipment").value;
  const sensorType = document.getElementById("input-sensor-type").value;
  const value = document.getElementById("input-value").value;
  const unit = document.getElementById("input-unit").value;
  const recordedAt = document.getElementById("input-recorded-at").value;

  const payload = {
    equipment_id: Number(equipmentId),
    sensor_type: sensorType,
    value: Number(value),
  };
  if (unit) payload.unit = unit;
  const recordedAtIso = localInputToIso(recordedAt);
  if (recordedAtIso) payload.recorded_at = recordedAtIso;

  try {
    await apiRequest("/readings", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    feedback.textContent = "Reading saved.";
    feedback.className = "feedback success";
    document.getElementById("reading-form").reset();
    await loadReadings();
    await loadStats();
  } catch (err) {
    feedback.textContent = `Error: ${err.message}`;
    feedback.className = "feedback error";
  }
}

function init() {
  document.getElementById("reading-form").addEventListener("submit", submitReading);
  document.getElementById("apply-filters").addEventListener("click", loadReadings);
  document.getElementById("refresh-readings").addEventListener("click", loadReadings);
  document.getElementById("apply-stats-filters").addEventListener("click", loadStats);

  checkHealth();
  loadEquipment().then(() => {
    loadReadings();
    loadStats();
  });
}

document.addEventListener("DOMContentLoaded", init);
