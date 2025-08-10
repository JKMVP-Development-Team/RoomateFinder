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
} from '@mui/material';
import reactLogo from './assets/react.svg';
import viteLogo from '/vite.svg';
import './App.css';

function App() {
  const [username, setUsername] = useState('');
  const [location, setLocation] = useState('');
  const [budget, setBudget] = useState([500, 1200]);
  const [cleanliness, setCleanliness] = useState(3);
  const [cleanlinessHover, setCleanlinessHover] = useState(-1);
  const [smoking, setSmoking] = useState('non-smoker');
  const [hasPets, setHasPets] = useState(false);
  const [petFriendly, setPetFriendly] = useState(false);

  const getCleanlinessLabel = value => {
    switch (value) {
      case 1:
        return 'Very relaxed about cleanliness';
      case 2:
        return 'Somewhat relaxed';
      case 3:
        return 'Moderately clean';
      case 4:
        return 'Very clean';
      case 5:
        return 'Extremely clean and organized';
      default:
        return 'Hover over stars to see descriptions';
    }
  };

  const handleSaveProfile = () => {
    const profileData = {
      username,
      location,
      budget,
      cleanliness,
      smoking,
      hasPets,
      petFriendly,
    };

    console.log('Profile Data:', profileData);
    alert('Profile saved! Check the console to see your data.');
  };

  return (
    <Container maxWidth="lg" sx={{ py: 4 }}>
      <Typography variant="h3" component="h1" gutterBottom align="center">
        🏠 Create Your Roommate Profile
      </Typography>

      <Grid container spacing={4} sx={{ mt: 2 }}>
        {/* Left side - Placeholder for future profile preview */}
        <Grid item xs={12} md={5}>
          <Paper
            sx={{
              p: 3,
              height: 'fit-content',
              backgroundColor: 'grey.50',
              border: '2px dashed',
              borderColor: 'grey.300',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              minHeight: 400,
              position: 'sticky',
              top: 16,
              zIndex: 1,
            }}
          >
            <Box textAlign="center">
              <Typography variant="h6" color="text.secondary" gutterBottom>
                Placeholder Profile Preview
              </Typography>
              <Typography variant="body2" color="text.secondary">
                Your profile card will appear here
              </Typography>
            </Box>
          </Paper>
        </Grid>

        {/* Right side - Edit form */}
        <Grid item xs={12} md={7}>
          <Card sx={{ p: 3 }}>
            <CardContent>
              <Typography variant="h6" gutterBottom>
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
                  sx={{ mt: 1 }}
                />
              </Box>
            </CardContent>
          </Card>

          <Card sx={{ mt: 3, p: 3 }}>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Lifestyle Preferences
              </Typography>
              <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
                Help us find compatible roommates for you
              </Typography>

              <Box sx={{ mb: 3 }}>
                <Typography component="legend" gutterBottom>
                  Cleanliness Level
                </Typography>
                <Rating
                  value={cleanliness}
                  onChange={(event, newValue) => setCleanliness(newValue)}
                  onChangeActive={(event, newHover) =>
                    setCleanlinessHover(newHover)
                  }
                  size="large"
                  precision={1}
                />
                <Typography
                  variant="body2"
                  color="text.secondary"
                  sx={{ mt: 1 }}
                >
                  {getCleanlinessLabel(
                    cleanlinessHover !== -1 ? cleanlinessHover : cleanliness
                  )}
                </Typography>
              </Box>

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
                py: 1.5,
                fontSize: '1.1rem',
                borderRadius: 2,
              }}
            >
              Save Profile
            </Button>
          </Box>
        </Grid>
      </Grid>
    </Container>
  );
}

export default App;
