import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import './index.css';
import App from './App';
import ChatPage from './Pages/ChatPage'; // Import the new page

createRoot(document.getElementById('root')).render(
  <StrictMode>
    <Router>
      <Routes>
        {/* Existing App (homepage) */}
        <Route path="/" element={<App />} />
        {/* New Chat Page */}
        <Route path="/chat" element={<ChatPage />} />
      </Routes>
    </Router>
  </StrictMode>
);