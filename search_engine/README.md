# LiteWave Aggregated Search Engine Module

ยินดีต้อนรับ! โฟลเดอร์ `search_engine/` นี้ถูกจัดเตรียมไว้สำหรับพัฒนา **Search Engine Server / API สรุปและรวมผลการค้นหา** ของ LiteWave

---

## 🔌 การเชื่อมต่อกับ LiteWave Browser

เบราว์เซอร์ LiteWave จะส่งคำค้นหาและดึงคำแนะนำการค้นหาผ่าน HTTP/HTTPS API ดังนี้:

### 1. Search Query Endpoint (หน้าแสดงผลค้นหา)
- **URL Format**: `http://localhost:8080/search?q={query}` (หรือ Domain ของ Server ที่ Deploy)
- **Response**: หน้าเว็บ HTML ผลการค้นหา

### 2. Autocomplete Suggestions API (คำแนะนำค้นหาอัตโนมัติ)
- **URL Format**: `http://localhost:8080/api/suggest?q={query}`
- **Response Format**: JSON Array รูปแบบมาตรฐาน OpenSearch:
```json
[
  "keyword",
  ["keyword suggestion 1", "keyword suggestion 2", "keyword suggestion 3"]
]
```

---

## 🛠️ คำแนะนำการเริ่มต้นพัฒนา
สามารถเลือกใช้ ภาษา/Framework ตามที่ถนัดได้เลย เช่น:
- **Node.js / Express / Fastify**
- **Python / FastAPI / Flask**
- **Go / Gin**
- **Rust / Actix-web**
