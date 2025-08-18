import { useState } from 'react';
import {
  Typography,
  Container,
  Button,
  Card,
  CardContent,
  TextField,
  Slider,
  Box,
  Rating,
  RadioGroup,
  FormControlLabel,
  Radio,
  FormControl,
  FormLabel,
  Switch,
  Grid,
  Paper,
  AppBar,
  Toolbar,
  IconButton,
} from '@mui/material';
import './App.css';
import UserCard from './components/UserCard';
import ProfileBrowser from './components/ProfileBrowser';

function App() {
  const [currentPage, setCurrentPage] = useState('create'); // 'create' or 'browse'
  const [username, setUsername] = useState('');
  const [location, setLocation] = useState('');
  const [bio, setBio] = useState('');
  const [budget, setBudget] = useState([500, 1200]);
  const [cleanliness, setCleanliness] = useState(3);
  const [smoking, setSmoking] = useState('non-smoker');
  const [hasPets, setHasPets] = useState(false);
  const [petFriendly, setPetFriendly] = useState(false);
  const [workSchedule, setWorkSchedule] = useState('9-5');
  const [socialLevel, setSocialLevel] = useState(3);
  const [noiseTolerance, setNoiseTolerance] = useState(3);

  // Rating fields configuration for reusability
  const ratingFields = [
    {
      key: 'cleanliness',
      label: 'Cleanliness Level',
      description: '1 = Very relaxed • 5 = Extremely clean and organized',
      value: cleanliness,
      setter: setCleanliness,
    },
    {
      key: 'socialLevel',
      label: 'Social Level',
      description: '1 = Prefer quiet/alone time • 5 = Love social gatherings',
      value: socialLevel,
      setter: setSocialLevel,
    },
    {
      key: 'noiseTolerance',
      label: 'Noise Tolerance',
      description: "1 = Need quiet environment • 5 = Don't mind noise",
      value: noiseTolerance,
      setter: setNoiseTolerance,
    },
  ];

  const handleSaveProfile = async () => {
    const profileData = {
      username,
      location,
      bio,
      budget,
      cleanliness,
      smoking,
      hasPets,
      petFriendly,
      workSchedule,
      socialLevel,
      noiseTolerance,
    };

    try {
      console.log('Sending profile data:', profileData);

      const response = await fetch('/api/profiles', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(profileData),
      });

      const result = await response.json();

      if (response.ok) {
        console.log('Profile saved successfully:', result);
        alert(`Profile saved successfully! ID: ${result.id}`);
      } else {
        console.error('Error saving profile:', result);
        alert(`Error saving profile: ${result.error || 'Unknown error'}`);
      }
    } catch (error) {
      console.error('Network error:', error);
      alert(
        'Failed to connect to server. Make sure the backend is running on port 18080.'
      );
    }
  };

  return (
    <Box
      sx={{
        minHeight: '100vh',
        backgroundColor: '#f5f5f5', // Light gray background
      }}
    >
      {/* Navigation Bar */}
      <AppBar position="static" sx={{ mb: 0 }}>
        <Toolbar>
          <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
            🏠 Roommate Finder
          </Typography>
          <Button
            color="inherit"
            sx={{ mr: 2 }}
            onClick={() => setCurrentPage('browse')}
            variant={currentPage === 'browse' ? 'outlined' : 'text'}
          >
            Browse Profiles
          </Button>
          <Button
            color="inherit"
            sx={{ mr: 2 }}
            onClick={() => setCurrentPage('create')}
            variant={currentPage === 'create' ? 'outlined' : 'text'}
          >
            Create Profile
          </Button>
          <Button color="inherit" sx={{ mr: 2 }}>
            Messages
          </Button>
          <Button color="inherit">Settings</Button>
        </Toolbar>
      </AppBar>

      {currentPage === 'browse' ? (
        <ProfileBrowser />
      ) : (
        <Container maxWidth="xl" sx={{ py: 2 }}>
          <Typography
            variant="h2"
            component="h1"
            gutterBottom
            align="center"
            sx={{
              fontWeight: 'bold',
              background: 'linear-gradient(45deg, #2196F3 30%, #21CBF3 90%)',
              backgroundClip: 'text',
              WebkitBackgroundClip: 'text',
              WebkitTextFillColor: 'transparent',
              mb: 1,
            }}
          >
            🏠 Roommate Finder
          </Typography>
          <Typography
            variant="h5"
            align="center"
            color="text.secondary"
            sx={{ mb: 4 }}
          >
            Create your perfect roommate profile
          </Typography>

          <Grid container spacing={4} sx={{ mt: 2 }}>
            {/* Left side - Profile preview using UserCard component */}
            <Grid
              sx={{
                minWidth: '305px',
                position: 'sticky',
                top: 24,
                zIndex: 1,
                height: 'fit-content',
              }}
            >
              <UserCard
                profileData={{
                  username,
                  location,
                  bio,
                  budget,
                  cleanliness,
                  smoking,
                  hasPets,
                  petFriendly,
                  workSchedule,
                  socialLevel,
                  noiseTolerance,
                }}
              />
            </Grid>

            {/* Right side - Edit form */}
            <Grid sx={{ flex: 1 }}>
              <Card sx={{ p: 3 }}>
                <CardContent>
                  <Typography variant="h5" gutterBottom>
                    Personal Information
                  </Typography>
                  <TextField
                    fullWidth
                    label="Username"
                    value={username}
                    onChange={e => setUsername(e.target.value)}
                    margin="normal"
                    placeholder="Enter your username"
                    helperText="This will be visible to potential roommates"
                  />
                  <TextField
                    fullWidth
                    label="Location"
                    value={location}
                    onChange={e => setLocation(e.target.value)}
                    margin="normal"
                    placeholder="e.g., Downtown Seattle, Capitol Hill"
                    helperText="Where are you looking for housing?"
                  />

                  <TextField
                    fullWidth
                    multiline
                    rows={2}
                    label="Bio"
                    value={bio}
                    onChange={e => setBio(e.target.value)}
                    margin="normal"
                    placeholder="Tell potential roommates about yourself..."
                    helperText="Share your interests, hobbies, and what you're looking for in a roommate"
                  />

                  <Box sx={{ mt: 2, mb: 2 }}>
                    <Typography gutterBottom>
                      Budget Range: ${budget[0]} - ${budget[1]} per month
                    </Typography>
                    <Slider
                      value={budget}
                      onChange={(e, newValue) => setBudget(newValue)}
                      valueLabelDisplay="auto"
                      min={500}
                      max={3000}
                      step={50}
                      marks={[
                        { value: 500, label: '$500' },
                        { value: 1000, label: '$1000' },
                        { value: 2000, label: '$2000' },
                        { value: 3000, label: '$3000' },
                      ]}
                      sx={{ width: '95%', mt: 1 }}
                    />
                  </Box>
                </CardContent>
              </Card>

              <Card sx={{ mt: 3, p: 3 }}>
                <CardContent>
                  <Typography variant="h5" gutterBottom>
                    Lifestyle Preferences
                  </Typography>
                  <Typography
                    variant="body2"
                    color="text.secondary"
                    sx={{ mb: 3 }}
                  >
                    Help us find compatible roommates for you
                  </Typography>

                  {ratingFields.map(field => (
                    <Box key={field.key} sx={{ mb: 3 }}>
                      <Typography component="legend" gutterBottom>
                        {field.label}
                      </Typography>
                      <Rating
                        value={field.value}
                        onChange={(event, newValue) => field.setter(newValue)}
                        size="large"
                        precision={1}
                      />
                      <Typography
                        variant="body2"
                        color="text.secondary"
                        sx={{ mt: 1 }}
                      >
                        {field.description}
                      </Typography>
                    </Box>
                  ))}

                  <FormControl component="fieldset" sx={{ mb: 3 }}>
                    <FormLabel component="legend">Smoking Preference</FormLabel>
                    <RadioGroup
                      value={smoking}
                      onChange={e => setSmoking(e.target.value)}
                      row
                    >
                      <FormControlLabel
                        value="non-smoker"
                        control={<Radio />}
                        label="Non-smoker"
                      />
                      <FormControlLabel
                        value="smoker"
                        control={<Radio />}
                        label="Smoker"
                      />
                      <FormControlLabel
                        value="no-preference"
                        control={<Radio />}
                        label="No preference"
                      />
                    </RadioGroup>
                  </FormControl>

                  <FormControl component="fieldset" sx={{ mb: 3 }}>
                    <FormLabel component="legend">Work Schedule</FormLabel>
                    <RadioGroup
                      value={workSchedule}
                      onChange={e => setWorkSchedule(e.target.value)}
                      row
                    >
                      <FormControlLabel
                        value="9-5"
                        control={<Radio />}
                        label="9-5 (Day shift)"
                      />
                      <FormControlLabel
                        value="night"
                        control={<Radio />}
                        label="Night shift"
                      />
                      <FormControlLabel
                        value="flexible"
                        control={<Radio />}
                        label="Flexible/Remote"
                      />
                    </RadioGroup>
                  </FormControl>

                  <Box sx={{ mb: 3 }}>
                    <Typography variant="h6" gutterBottom>
                      Pet Information
                    </Typography>
                    <FormControlLabel
                      control={
                        <Switch
                          checked={hasPets}
                          onChange={e => setHasPets(e.target.checked)}
                          color="primary"
                        />
                      }
                      label="I have pets"
                      sx={{ display: 'block', mb: 2 }}
                    />
                    <FormControlLabel
                      control={
                        <Switch
                          checked={petFriendly}
                          onChange={e => setPetFriendly(e.target.checked)}
                          color="primary"
                        />
                      }
                      label="Pet-friendly roommate welcome"
                      sx={{ display: 'block', mb: 2 }}
                    />
                  </Box>
                </CardContent>
              </Card>

              <Box sx={{ mt: 4, display: 'flex', justifyContent: 'center' }}>
                <Button
                  variant="contained"
                  size="large"
                  onClick={handleSaveProfile}
                  sx={{
                    px: 6,
                    py: 2,
                    fontSize: '1.2rem',
                    borderRadius: 3,
                  }}
                >
                  Save Profile
                </Button>
              </Box>
            </Grid>
          </Grid>
        </Container>
      )}
    </Box>
  );
}

export default App;
