/**
 * Back2You - Lightweight Vanilla JS API Client
 * Designed & Developed by Aditya Rai & Sameer Chand
 */

const API_BASE = window.location.origin.includes('8080') 
  ? '/api' 
  : 'http://localhost:8080/api';

const Back2You = {
  // Check backend server health
  async checkHealth() {
    try {
      const res = await fetch(`${API_BASE}/health`);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      console.log('[Back2You] Connected to C++ backend:', data);
      return data;
    } catch (err) {
      console.warn('[Back2You] Running in standalone frontend mode (C++ server offline):', err.message);
      return null;
    }
  },

  // Fetch categories from database
  async getCategories() {
    try {
      const res = await fetch(`${API_BASE}/categories`);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      return data.categories || [];
    } catch (err) {
      console.warn('[Back2You] Error fetching categories:', err);
      return [];
    }
  },

  // Search items via REST API
  async searchItems(params = {}) {
    try {
      const query = new URLSearchParams(params).toString();
      const res = await fetch(`${API_BASE}/items?${query}`);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      return data.items || [];
    } catch (err) {
      console.warn('[Back2You] Error fetching items:', err);
      return [];
    }
  }
};

// Initialize on load
document.addEventListener('DOMContentLoaded', () => {
  Back2You.checkHealth();
});
