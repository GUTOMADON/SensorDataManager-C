# SensorDataManager-C Frontend

A dependency-free static dashboard (plain HTML, CSS, and JavaScript) that
consumes the [backend API](../backend/README.md). No build step, no
package manager, and no framework.

## Files

```
frontend/
├── index.html     Page structure: submit form, readings table, stats table
├── css/style.css    Styling
└── js/app.js         Fetch calls to the API and DOM rendering
```

## How it talks to the backend

`js/app.js` defines a single constant at the top of the file:

```js
const API_BASE_URL = "http://localhost:8080/api";
```

Every request the dashboard makes (`GET /equipment`, `GET /readings`,
`GET /stats`, `POST /readings`) is built on top of this base URL using the
browser's `fetch()` API. If the backend runs on a different host or port,
edit this one constant.

The backend sends `Access-Control-Allow-Origin: *` on every response
(see `backend/src/http_server.c`), so the dashboard can be opened directly
from the filesystem or served from any local static file server without
running into cross-origin errors.

## Running it

The dashboard is static, so any of the following work. Run the backend
first (see [backend/README.md](../backend/README.md)), then serve the
frontend with one of:

**Option 1: open the file directly**

Open `frontend/index.html` in a browser (double-click it, or use your
OS's "open with browser" action).

**Option 2: serve it with Python's built-in server**

```bash
cd frontend
python3 -m http.server 5500
```

Then visit `http://localhost:5500` in a browser.

**Option 3: serve it with Node's `npx serve`**

```bash
cd frontend
npx serve -l 5500
```

## What the dashboard does

- **Submit a Reading**: a form that posts a new sensor reading to
  `POST /api/readings`. The equipment dropdown is populated from
  `GET /api/equipment`; the unit field auto-fills server-side if left blank.
- **Readings**: a filterable table backed by `GET /api/readings`, with
  filters for equipment, sensor type, time range, and result limit.
- **Aggregate Statistics**: a table backed by `GET /api/stats` showing the
  average, minimum, maximum, and sample count per equipment/sensor
  combination, filterable by equipment and sensor type.
- A status indicator in the header pings `GET /api/health` on page load to
  show whether the API and database are reachable.
