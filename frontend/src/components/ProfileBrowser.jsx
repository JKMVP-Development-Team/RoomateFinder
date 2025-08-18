import { useState, useEffect } from 'react';
import {
  Container,
  Typography,
  Grid,
  Box,
  Button,
  Card,
  CardContent,
  CircularProgress,
  Alert,
} from '@mui/material';
import UserCard from './UserCard';

function ProfileBrowser() {
  const [profiles, setProfiles] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    fetchProfiles();
  }, []);

  const fetchProfiles = async () => {
    try {
      setLoading(true);
      const response = await fetch('/api/profiles');

      if (response.ok) {
        const data = await response.json();
        console.log('Fetched profiles data:', data);
        setProfiles(data.profiles || []);
      } else {
        setError('Failed to fetch profiles');
      }
    } catch (err) {
      setError(
        'Failed to connect to server. Make sure the backend is running.'
      );
      console.error('Error fetching profiles:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleRefresh = () => {
    fetchProfiles();
  };

  if (loading) {
    return (
      <Container maxWidth="xl" sx={{ py: 4, textAlign: 'center' }}>
        <CircularProgress />
        <Typography variant="h6" sx={{ mt: 2 }}>
          Loading profiles...
        </Typography>
      </Container>
    );
  }

  if (error) {
    return (
      <Container maxWidth="xl" sx={{ py: 4 }}>
        <Alert severity="error" sx={{ mb: 2 }}>
          {error}
        </Alert>
        <Button variant="contained" onClick={handleRefresh}>
          Try Again
        </Button>
      </Container>
    );
  }

  return (
    <Container maxWidth="xl" sx={{ py: 4 }}>
      <Box
        sx={{
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center',
          mb: 4,
        }}
      >
        <Typography variant="h4" component="h1" gutterBottom color="primary">
          🏠 Saved Roommate Profiles ({profiles.length})
        </Typography>
        <Button variant="outlined" onClick={handleRefresh}>
          Refresh
        </Button>
      </Box>

      {profiles.length === 0 ? (
        <Card>
          <CardContent sx={{ textAlign: 'center', py: 6 }}>
            <Typography variant="h6" color="text.secondary" gutterBottom>
              No profiles found
            </Typography>
            <Typography variant="body2" color="text.secondary">
              Create a profile first, then come back here to see it displayed!
            </Typography>
          </CardContent>
        </Card>
      ) : (
        <Grid container spacing={3}>
          {profiles.map((profile, index) => (
            <Grid item xs={12} sm={6} md={4} lg={3} key={profile.id || index}>
              <UserCard profileData={profile} />
            </Grid>
          ))}
        </Grid>
      )}
    </Container>
  );
}

export default ProfileBrowser;
