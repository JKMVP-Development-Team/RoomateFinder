// src/pages/HomePage.jsx
import React, { useEffect, useState } from "react";
import SwipeCard from "../components/SwipeCard";
import "./HomePage.css";

export default function HomePage() {
  const [profiles, setProfiles] = useState([]);
  const [currentIndex, setCurrentIndex] = useState(0);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  // Replace with the current logged-in user id
  // Hardcoded for now
  const currentUserId = "YOUR_USER_ID_HERE";

  // Fetch recommended roommates from backend
  useEffect(() => {
    async function fetchRecommendations() {
      setLoading(true);
      setError("");
      try {
        const res = await fetch(`/api/recommend?type=roommate&userId=${currentUserId}`);
        if (!res.ok) throw new Error("Failed to fetch recommendations");
        const data = await res.json();
        if (data.error) throw new Error(data.error);
        setProfiles(data.entity || []);
      } catch (err) {
        setError(err.message);
      } finally {
        setLoading(false);
      }
    }
    fetchRecommendations();
  }, [currentUserId]);

  const handleSwipe = async (isLike) => {
    if (currentIndex >= profiles.length) return;

    const profile = profiles[currentIndex];
    try {
      const res = await fetch("/api/swipe", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          type: "roommate",
          sourceId: currentUserId,
          targetId: profile.id,
          isLike,
        }),
      });
      if (!res.ok) throw new Error("Failed to send swipe");
    } catch (err) {
      console.error(err);
      // Error feedback here
    }

    setCurrentIndex((i) => i + 1);
  };

  if (loading) return <div className="loading">Loading recommendations...</div>;
  if (error) return <div className="error">Error: {error}</div>;
  if (currentIndex >= profiles.length)
    return <div className="no-more">No more profiles to show</div>;

  // Construct profile object expected by SwipeCard
  const profile = {
    name: profiles[currentIndex].username || "Unnamed",
    age: profiles[currentIndex].age,
    bio: `${profiles[currentIndex].city}, ${profiles[currentIndex].state}`,
  };

  return (
    <div className="homepage">
      <SwipeCard
        profile={profile}
        onLike={() => handleSwipe(true)}
        onDislike={() => handleSwipe(false)}
      />
    </div>
  );
}
