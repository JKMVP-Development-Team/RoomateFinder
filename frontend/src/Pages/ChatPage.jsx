import { useState, useEffect, useRef } from 'react';
import './ChatPage.css';

export default function ChatPage() {
  // State and refs
  const [messages, setMessages] = useState([]);
  const [newMessage, setNewMessage] = useState('');
  const [currentUser, setCurrentUser] = useState(
    localStorage.getItem('chatUsername') || `User${Math.floor(Math.random() * 1000)}`
  );
  const [activeConversation, setActiveConversation] = useState(null);
  const messagesEndRef = useRef(null);

  // Sample data
  const [conversations] = useState([
    { id: '1', userId: 'user2', name: 'Alex Johnson', lastMessage: 'Hey, are you still looking for a roommate?' },
    { id: '2', userId: 'user3', name: 'Sam Wilson', lastMessage: 'I sent you the apartment details' },
    { id: '3', userId: 'user4', name: 'Taylor Smith', lastMessage: 'When can you come for a visit?' },
  ]);

  // Send message handler
  const sendMessage = () => {
    if (!newMessage.trim() || !activeConversation) return;
    
    // In a real app, you would send to your backend here
    const newMsg = {
      id: Date.now(),
      text: newMessage,
      sender: currentUser,
      timestamp: new Date()
    };
    
    setMessages(prev => [...prev, newMsg]);
    setNewMessage('');
  };

  // Auto-scroll to bottom
  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  return (
    <div className="chat-container">
      {/* Sidebar */}
      <div className="sidebar">
        <div className="user-profile">
          <div className="user-avatar">
            {currentUser.charAt(0).toUpperCase()}
          </div>
          <input
            type="text"
            value={currentUser}
            onChange={(e) => setCurrentUser(e.target.value)}
            className="username-input"
            placeholder="Your name"
          />
        </div>
        
        <div className="conversations">
          {conversations.map(convo => (
            <div 
              key={convo.id}
              className={`conversation-item ${activeConversation?.id === convo.id ? 'active' : ''}`}
              onClick={() => setActiveConversation(convo)}
            >
              <div className="conversation-avatar">
                {convo.name.charAt(0).toUpperCase()}
              </div>
              <div className="conversation-info">
                <div className="conversation-name">{convo.name}</div>
                <div className="conversation-preview">{convo.lastMessage}</div>
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* Main Chat Area */}
      <div className="chat-area">
        {activeConversation ? (
          <>
            <div className="chat-header">
              <div className="conversation-avatar">
                {activeConversation.name.charAt(0).toUpperCase()}
              </div>
              <div className="conversation-info">
                <div className="conversation-name">{activeConversation.name}</div>
              </div>
            </div>
            
            <div className="messages-container">
              {messages.length === 0 ? (
                <div className="empty-state">
                  <p>No messages yet</p>
                  <p>Send a message to start chatting</p>
                </div>
              ) : (
                messages.map(msg => (
                  <div 
                    key={msg.id}
                    className={`message ${msg.sender === currentUser ? 'sent' : 'received'}`}
                  >
                    <div className="message-content">{msg.text}</div>
                  </div>
                ))
              )}
              <div ref={messagesEndRef} />
            </div>

            <div className="message-input-container">
              <div className="message-input">
                <input
                  type="text"
                  value={newMessage}
                  onChange={(e) => setNewMessage(e.target.value)}
                  onKeyPress={(e) => e.key === 'Enter' && sendMessage()}
                  placeholder="Type a message..."
                />
                <button onClick={sendMessage}>
                  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor">
                    <path d="M22 2L11 13M22 2l-7 20-4-9-9-4 20-7z" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"/>
                  </svg>
                </button>
              </div>
            </div>
          </>
        ) : (
          <div className="empty-state">
            <p>Select a conversation to start chatting</p>
          </div>
        )}
      </div>
    </div>
  );
}