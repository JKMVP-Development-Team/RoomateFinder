import React from "react";
import "./SwipeCard.css";

export default function SwipeCard({ profile, onLike, onDislike }) {
  return (
    <div className="swipe-card">
      <h2>{profile.name}</h2>
      {profile.age && <p>Age: {profile.age}</p>}
      {profile.bio && <p>{profile.bio}</p>}
      <div style={{ marginTop: "1rem" }}>
        <button className="dislike" onClick={onDislike}>❌ Dislike</button>
        <button className="like" onClick={onLike}>💖 Like</button>
      </div>
    </div>
  );
}
